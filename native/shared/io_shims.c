#include <_ansi.h>
#include <unistd.h>
#include <reent.h>

ssize_t
_pwrite_r (struct _reent *rptr,
     int fd,
     const void *buf,
     size_t n,
     off_t off)
{
  off_t cur_pos;
  _READ_WRITE_RETURN_TYPE num_written;
  
  if ((cur_pos = _lseek_r (rptr, fd, 0, SEEK_CUR)) == (off_t)-1)
    return -1;

  if (_lseek_r (rptr, fd, off, SEEK_SET) == (off_t)-1)
    return -1;

  num_written = _write_r (rptr, fd, buf, n);

  if (_lseek_r (rptr, fd, cur_pos, SEEK_SET) == (off_t)-1)
    return -1;

  return (ssize_t)num_written;
}

ssize_t
pwrite (int fd,
     const void *buf,
     size_t n,
     off_t off)
{
  return _pwrite_r (_REENT, fd, buf, n, off);
}
ssize_t
_pread_r (struct _reent *rptr,
     int fd,
     void *buf,
     size_t n,
     off_t off)
{
  off_t cur_pos;
  _READ_WRITE_RETURN_TYPE num_read;
  
  if ((cur_pos = _lseek_r (rptr, fd, 0, SEEK_CUR)) == (off_t)-1)
    return -1;

  if (_lseek_r (rptr, fd, off, SEEK_SET) == (off_t)-1)
    return -1;

  num_read = _read_r (rptr, fd, buf, n);

  if (_lseek_r (rptr, fd, cur_pos, SEEK_SET) == (off_t)-1)
    return -1;

  return (ssize_t)num_read;
}

ssize_t
pread (int fd,
     void *buf,
     size_t n,
     off_t off)
{
  return _pread_r (_REENT, fd, buf, n, off);
}