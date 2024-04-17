
#include "kdk/amd64/portio.h"
#include "kdk/kmem.h"
#include "kdk/nanokern.h"
#include "kdk/vm.h"
#include "uacpi/kernel_api.h"
#include "uacpi/status.h"

#ifdef AMD64
void
laihost_outb(uint16_t port, uint8_t val)
{
	asm volatile("outb %0, %1" : : "a"(val), "d"(port));
}
void
laihost_outw(uint16_t port, uint16_t val)
{
	asm volatile("outw %0, %1" : : "a"(val), "d"(port));
}
void
laihost_outd(uint16_t port, uint32_t val)
{
	asm volatile("outl %0, %1" : : "a"(val), "d"(port));
}

uint8_t
laihost_inb(uint16_t port)
{
	uint8_t val;
	asm volatile("inb %1, %0" : "=a"(val) : "d"(port));
	return val;
}
uint16_t
laihost_inw(uint16_t port)
{
	uint16_t val;
	asm volatile("inw %1, %0" : "=a"(val) : "d"(port));
	return val;
}
uint32_t
laihost_ind(uint16_t port)
{
	uint32_t val;
	asm volatile("inl %1, %0" : "=a"(val) : "d"(port));
	return val;
}

uint8_t
pci_readb(uint32_t bus, uint32_t slot, uint32_t function, uint32_t offset)
{
	uint32_t address = (bus << 16) | (slot << 11) | (function << 8) |
	    (offset & ~(uint32_t)(3)) | 0x80000000;
	outl(0xCF8, address);
	return inb(0xCFC + (offset & 3));
}

uint16_t
pci_readw(uint32_t bus, uint32_t slot, uint32_t function, uint32_t offset)
{
	uint32_t address = (bus << 16) | (slot << 11) | (function << 8) |
	    (offset & ~(uint32_t)(3)) | 0x80000000;
	outl(0xCF8, address);
	return inw(0xCFC + (offset & 3));
}

uint32_t
pci_readl(uint32_t bus, uint32_t slot, uint32_t function, uint32_t offset)
{
	uint32_t address = (bus << 16) | (slot << 11) | (function << 8) |
	    (offset & ~(uint32_t)(3)) | 0x80000000;
	outl(0xCF8, address);
	return inl(0xCFC + (offset & 3));
}

void
pci_writeb(uint32_t bus, uint32_t slot, uint32_t function, uint32_t offset,
    uint8_t value)
{
	uint32_t address = (bus << 16) | (slot << 11) | (function << 8) |
	    (offset & ~(uint32_t)(3)) | 0x80000000;
	outl(0xCF8, address);
	outb(0xCFC + (offset & 3), value);
}

void
pci_writew(uint32_t bus, uint32_t slot, uint32_t function, uint32_t offset,
    uint16_t value)
{
	uint32_t address = (bus << 16) | (slot << 11) | (function << 8) |
	    (offset & ~(uint32_t)(3)) | 0x80000000;
	outl(0xCF8, address);
	outw(0xCFC + (offset & 3), value);
}

void
pci_writel(uint32_t bus, uint32_t slot, uint32_t function, uint32_t offset,
    uint32_t value)
{
	uint32_t address = (bus << 16) | (slot << 11) | (function << 8) |
	    (offset & ~(uint32_t)(3)) | 0x80000000;
	outl(0xCF8, address);
	outl(0xCFC + (offset & 3), value);
}
#elif 0
uint8_t
pci_readb(uint32_t bus, uint32_t slot, uint32_t function, uint32_t offset)
{
}

uint16_t
pci_readw(uint32_t bus, uint32_t slot, uint32_t function, uint32_t offset)
{
}

uint32_t
pci_readl(uint32_t bus, uint32_t slot, uint32_t function, uint32_t offset)
{
}

void
pci_writeb(uint32_t bus, uint32_t slot, uint32_t function, uint32_t offset,
    uint8_t value)
{
}

void
pci_writew(uint32_t bus, uint32_t slot, uint32_t function, uint32_t offset,
    uint16_t value)
{
}

void
pci_writel(uint32_t bus, uint32_t slot, uint32_t function, uint32_t offset,
    uint32_t value)
{
}
#endif

void
laihost_pci_writeb(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fn,
    uint16_t offset, uint8_t v)
{
	kassert(seg == 0);
	return pci_writeb(bus, slot, fn, offset, v);
}
void
laihost_pci_writew(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fn,
    uint16_t offset, uint16_t v)
{
	kassert(seg == 0);
	return pci_writew(bus, slot, fn, offset, v);
}
void
laihost_pci_writed(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fn,
    uint16_t offset, uint32_t v)
{
	kassert(seg == 0);
	return pci_writel(bus, slot, fn, offset, v);
}

uint8_t
laihost_pci_readb(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fn,
    uint16_t offset)
{
	kassert(seg == 0);
	return pci_readb(bus, slot, fn, offset);
}
uint16_t
laihost_pci_readw(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fn,
    uint16_t offset)
{
	kassert(seg == 0);
	return pci_readw(bus, slot, fn, offset);
}
uint32_t
laihost_pci_readd(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fn,
    uint16_t offset)
{
	kassert(seg == 0);
	return pci_readl(bus, slot, fn, offset);
}

void
uacpi_kernel_vlog(enum uacpi_log_level lvl, const char *msg, uacpi_va_list va)
{
	const char *lvlStr;
	char buf[128];

	switch (lvl) {
	case UACPI_LOG_TRACE:
		lvlStr = "trace";
		break;
	case UACPI_LOG_INFO:
		lvlStr = "info";
		break;
	case UACPI_LOG_WARN:
		lvlStr = "warn";
		break;
	case UACPI_LOG_ERROR:
		lvlStr = "error";
		break;
	default:
		lvlStr = "<invalid>";
	}

	if (lvl <= UACPI_LOG_INFO) {
		kprintf("uacpi %s: ", lvlStr);
		kvpprintf(msg, va);
	}
}

void
uacpi_kernel_log(enum uacpi_log_level lvl, const char *fmt, ...)
{
	va_list va;
	va_start(va, fmt);

	uacpi_kernel_vlog(lvl, fmt, va);

	va_end(va);
}

void *
uacpi_kernel_alloc(uacpi_size size)
{
	return kmem_malloc(size);
}

void *
uacpi_kernel_calloc(uacpi_size count, uacpi_size size)
{
	return kmem_calloc(count, size);
}

void
uacpi_kernel_free(void *ptr)
{
	return kmem_mfree(ptr);
}

void *
uacpi_kernel_map(uacpi_phys_addr physical, uacpi_size length)
{
	return (void *)P2V(physical);
}

void
uacpi_kernel_unmap(void *ptr, uacpi_size length)
{
}

uacpi_status
uacpi_kernel_raw_memory_read(uacpi_phys_addr address, uacpi_u8 byte_width,
    uacpi_u64 *out)
{
	kfatal("Implement me\n");
}

uacpi_status
uacpi_kernel_raw_memory_write(uacpi_phys_addr address, uacpi_u8 byte_width,
    uacpi_u64 in)
{
	kfatal("Implement me\n");
}

#ifdef __amd64__
uacpi_status
uacpi_kernel_raw_io_write(uacpi_io_addr address, uacpi_u8 byte_width,
    uacpi_u64 in_value)
{
	switch (byte_width) {
	case 1:
		outb(address, in_value);
		break;

	case 2:
		outw(address, in_value);
		break;

	case 4:
		outl(address, in_value);
		break;

	default:
		kfatal("Unexpected\n");
	}

	return UACPI_STATUS_OK;
}

uacpi_status
uacpi_kernel_raw_io_read(uacpi_io_addr address, uacpi_u8 byte_width,
    uacpi_u64 *out_value)
{
	switch (byte_width) {
	case 1:
		*out_value = inb(address);
		break;

	case 2:
		*out_value = inw(address);
		break;

	case 4:
		*out_value = inl(address);
		break;

	default:
		kfatal("Unexpected\n");
	}

	return UACPI_STATUS_OK;
}
#else
uacpi_status
uacpi_kernel_raw_io_read(uacpi_io_addr, uacpi_u8, uacpi_u64 *)
{
	kfatal("Implement me\n");
	return UACPI_STATUS_UNIMPLEMENTED;
}

uacpi_status
uacpi_kernel_raw_io_write(uacpi_io_addr, uacpi_u8, uacpi_u64)
{
	kfatal("Implement me\n");
	return UACPI_STATUS_UNIMPLEMENTED;
}

#endif

uacpi_status
uacpi_kernel_io_map(uacpi_io_addr base, uacpi_size, uacpi_handle *out_handle)
{
	*out_handle = (uacpi_handle)base;
	return UACPI_STATUS_OK;
}
void
uacpi_kernel_io_unmap(uacpi_handle)
{
	kfatal("Implement me\n");
}

uacpi_status
uacpi_kernel_io_read(uacpi_handle handle, uacpi_size offset,
    uacpi_u8 byte_width, uacpi_u64 *value)
{
	return uacpi_kernel_raw_io_read((uacpi_io_addr)handle + offset,
	    byte_width, value);
}

uacpi_status
uacpi_kernel_io_write(uacpi_handle handle, uacpi_size offset,
    uacpi_u8 byte_width, uacpi_u64 value)
{
	return uacpi_kernel_raw_io_write((uacpi_io_addr)handle + offset,
	    byte_width, value);
}

uacpi_status
uacpi_kernel_pci_read(uacpi_pci_address *address, uacpi_size offset,
    uacpi_u8 byte_width, uacpi_u64 *value)
{
	switch (byte_width) {
	case 1:
		*value = pci_readb(address->bus, address->device,
		    address->function, offset);
		break;

	case 2:
		*value = pci_readw(address->bus, address->device,
		    address->function, offset);
		break;

	case 4:
		*value = pci_readl(address->bus, address->device,
		    address->function, offset);
		break;

	default:
		return UACPI_STATUS_INVALID_ARGUMENT;
	}

	return UACPI_STATUS_OK;
}

uacpi_status
uacpi_kernel_pci_write(uacpi_pci_address *address, uacpi_size offset,
    uacpi_u8 byte_width, uacpi_u64 value)
{
	switch (byte_width) {
	case 1:
		pci_writeb(address->bus, address->device,
		    address->function, offset, value);
		break;

	case 2:
		pci_writew(address->bus, address->device,
		    address->function, offset, value);
		break;

	case 4:
		pci_writel(address->bus, address->device,
		    address->function, offset, value);
		break;

	default:
		return UACPI_STATUS_INVALID_ARGUMENT;
	}

	return UACPI_STATUS_OK;
}

uacpi_u64
uacpi_kernel_get_ticks(void)
{
	kfatal("Implement me\n");
}

void
uacpi_kernel_stall(uacpi_u8 usec)
{
	kfatal("Implement me\n");
}

void
uacpi_kernel_sleep(uacpi_u64 msec)
{
	kfatal("Implement me\n");
}

uacpi_status
uacpi_kernel_install_interrupt_handler(uacpi_u32 irq,
    uacpi_interrupt_handler handler, uacpi_handle ctx,
    uacpi_handle *out_irq_handle)
{
	return UACPI_STATUS_OK;
}

uacpi_status
uacpi_kernel_uninstall_interrupt_handler(uacpi_interrupt_handler, uacpi_handle)
{
	kfatal("Implement me\n");
}

uacpi_status
uacpi_kernel_schedule_work(uacpi_work_type type, uacpi_work_handler handler,
    uacpi_handle ctx)
{
	kfatal("Implement me\n");
}

uacpi_status
uacpi_kernel_wait_for_work_completion(void)
{
	kfatal("Implement me\n");
}

uacpi_status
uacpi_kernel_handle_firmware_request(uacpi_firmware_request *req)
{
	kfatal("Implement me\n");
}

uacpi_handle
uacpi_kernel_create_mutex(void)
{
	kmutex_t *mutex = kmem_alloc(sizeof(kmutex_t));
	ke_mutex_init(mutex);
	return mutex;
}

void
uacpi_kernel_free_mutex(uacpi_handle opaque)
{
	kmem_free(opaque, sizeof(kmutex_t));
}

uacpi_bool
uacpi_kernel_acquire_mutex(uacpi_handle opaque, uacpi_u16 timeout)
{
	kmutex_t *mutex = opaque;
	kwaitstatus_t w = ke_wait(mutex, "uacpi_kernel_acquire_mutex", false,
	    false, timeout == 0xffff ? -1 : timeout);
	kassert(w == kKernWaitStatusOK || w == kKernWaitStatusTimedOut);
	return w == kKernWaitStatusOK ? true : false;
}

void
uacpi_kernel_release_mutex(uacpi_handle opaque)
{
	kmutex_t *mutex = opaque;
	ke_mutex_release(mutex);
}

uacpi_handle
uacpi_kernel_create_event(void)
{
	kfatal("Implement me\n");
}
void
uacpi_kernel_free_event(uacpi_handle opaque)
{
	kfatal("Implement me\n");
}

uacpi_bool
uacpi_kernel_wait_for_event(uacpi_handle opaque, uacpi_u16 timeout)
{
	kfatal("Implement me\n");
}

void
uacpi_kernel_signal_event(uacpi_handle opaque)
{
	kfatal("Implement me\n");
}
void
uacpi_kernel_reset_event(uacpi_handle opaque)
{
	kfatal("Implement me\n");
}