#ifndef FIO_SMALLOC_H
#define FIO_SMALLOC_H

extern void sinit(void);
extern void *smalloc(size_t size);

extern unsigned int smalloc_pool_size;

#endif
