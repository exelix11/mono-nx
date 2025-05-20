#include "io_util.h"

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <switch.h>

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include <sys/iosupport.h>

#include <unicode/utypes.h>
#include <unicode/udata.h>
#include <unicode/ucnv.h>
#include <unicode/ulocdata.h>
#include <unicode/uclean.h>

bool io_load_file(const char *path, uint8_t **out_data, size_t *out_size)
{
    FILE *file = fopen(path, "rb");
    if (!file)
        return false;

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);

    uint8_t *data = malloc(size);
    if (!data)
    {
        fclose(file);
        return false;
    }

    fread(data, 1, size, file);
    fclose(file);

    *out_data = data;
    *out_size = size;
    return true;
}

static uint8_t *icudt771_dat = NULL;
static size_t icudt771_dat_size = 0;

int io_init_libicu(const char* icudata_path, bool log)
{
    if (!io_load_file(icudata_path, &icudt771_dat, &icudt771_dat_size))
    {
        if (log) io_debugf("Failed to load ICU data file from %s\n", icudata_path);
        return 0;
    }

    UErrorCode status = U_ZERO_ERROR;

    // Initialize ICU with the embedded data
    udata_setCommonData(icudt771_dat, &status);
    if (U_FAILURE(status))
    {
        if (log) io_debugf("Failed to initialize ICU data: %s\n", u_errorName(status));
        return 0;
    }

    // Verify ICU is working
    const char *version = ucnv_getDefaultName();
    if (log) io_debugf("ICU initialized successfully. Default converter: %s\n", version);

    UVersionInfo cldrVersion;
    ulocdata_getCLDRVersion(cldrVersion, &status);
    if (U_FAILURE(status))
    {
        if (log) io_debugf("Failed to get CLDR version: %s\n", u_errorName(status));
        return 0;
    }

    if (log) io_debugf("CLDR version: %d.%d.%d.%d\n",
           cldrVersion[0], cldrVersion[1], cldrVersion[2], cldrVersion[3]);

    return 1;
}

void io_dispose_libicu()
{
    u_cleanup();

    // Free the ICU data
    if (icudt771_dat)
    {
        free(icudt771_dat);
        icudt771_dat = NULL;
        icudt771_dat_size = 0;
    }
}

typedef ssize_t (*newlib_io_write_callback)(struct _reent *r, void *fd, const char *ptr, size_t len);

static bool isCustomRedirect = false;
static bool isSvcRedirect = false;

bool io_has_stdio_redirection()
{
    return isCustomRedirect;
}

static ssize_t svc_log_write(struct _reent *r, void *fd, const char *ptr, size_t len)
{
    svcOutputDebugString(ptr, len);
    return len;
}

static int logUdpSocket = -1;
static struct sockaddr_in logUdpAddress;

static ssize_t udp_log_write(struct _reent *r, void *fd, const char *ptr, size_t len)
{
    if (logUdpSocket < 0) // THe socket has been closed, we're probably exiting
        return len;

    if (len == 0)
        return 0;

    // Send no more than 256 bytes at once
    for (size_t i = 0; i < len; i += 256)
    {
        size_t chunkSize = len - i > 256 ? 256 : len - i;
        sendto(logUdpSocket, ptr + i, chunkSize, 0, (struct sockaddr *)&logUdpAddress, sizeof(logUdpAddress));
    }

    return len;
}

static FILE* file_log_handle;
static ssize_t file_log_write(struct _reent *r, void *fd, const char *ptr, size_t len)
{
    if (!file_log_handle)
        return -1;

    if (len == 0)
        return 0;

    return fwrite(ptr, 1, len, file_log_handle);
}

static int redirect_newlib_io(newlib_io_write_callback callback)
{
    if (isCustomRedirect)
        return -1;

    isCustomRedirect = true;

    static devoptab_t dbgSvc = {
        .name = "ioredirect",
    };

    dbgSvc.write_r = callback;

    devoptab_list[STD_ERR] = &dbgSvc;
    devoptab_list[STD_OUT] = &dbgSvc;

    setvbuf(stderr, NULL, _IOLBF, 0);
    setvbuf(stdout, NULL, _IOLBF, 0);

    return 0;
}

int io_stdio_to_svc()
{
    if (redirect_newlib_io(svc_log_write) < 0)
        return -1;

    isSvcRedirect = true;

    printf("Enabled print to svc redirection\n");

    return 0;
}

void io_stdio_finish()
{
    if (logUdpSocket >= 0)
    {
        close(logUdpSocket);
        logUdpSocket = -1;
    }

    if (file_log_handle)
    {
        fclose(file_log_handle);
        file_log_handle = NULL;
    }
}

int io_stdio_to_udp(const char *host, int port)
{
    if (logUdpSocket >= 0)
        return -1;

    logUdpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (logUdpSocket < 0)
        return -1;

    memset(&logUdpAddress, 0, sizeof(logUdpAddress));
    logUdpAddress.sin_family = AF_INET;
    logUdpAddress.sin_port = htons(port);
    logUdpAddress.sin_addr.s_addr = inet_addr(host);

    if (redirect_newlib_io(udp_log_write) < 0)
    {
        close(logUdpSocket);
        logUdpSocket = -1;
        return -1;
    }

    return 0;
}

int io_stdio_to_file(const char* filename)
{
    file_log_handle = fopen(filename, "a");
    if (!file_log_handle)
        return -1;

    fprintf(file_log_handle, "--- starting log ---\n");

    if (redirect_newlib_io(file_log_write) < 0)
    {
        fclose(file_log_handle);
        file_log_handle = NULL;
        return -1;
    }

    return 0;
}

void io_debugf(const char *fmt, ...)
{
    char buffer[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    if (!isSvcRedirect)
        svcOutputDebugString(buffer, strlen(buffer));

    printf("%s\n", buffer);
}

char* io_strdup(const char* str)
{
    if (!str) return NULL;
    int len = strlen(str);

    char* res = malloc(len + 1);
    strcpy(res, str);

    return res;
}