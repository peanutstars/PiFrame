#ifndef ___PFU_FIFO_H___
#define ___PFU_FIFO_H___

typedef void PFUFifo ;

PFUFifo *PFU_createFIFO(int queueCount) ;
void PFU_destroyFIFO (PFUFifo *handle) ;
void PFU_enqueueFIFO (PFUFifo *handle, void *data, int size) ;
void *PFU_dequeueFIFO (PFUFifo *handle) ;

#endif /* ___PFU_FIFO_H___ */
