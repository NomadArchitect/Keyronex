kernel_sources += files(
	'pci/DKPCIBus.m',
	'virtio/DKVirtIOMMIOTransport.m',
	'virtio/DKVirtIOPCITransport.m',
	'virtio/VirtIO9pPort.m',
	'virtio/VirtIODisk.m',
	'virtio/VirtIOGPU.m',
	'9pSockTransport.m',
	'DKDevice.m',
	'DKNIC.m',
	'DKObject.m',
	'DOSFS.m',
	'Ext2FS.m',
	'FBTerminal.m',
	'Null.m',
	'SimpleFB.m',
	'buf.c',
	'iop.m',
	'setup_ddk.m',
)

kernel_sources += files(
	'../../vendor/limine-terminal/backends/framebuffer.c',
	'../../vendor/limine-terminal/term.c',
)

kernel_sources += [
	bin2c_gen.process('key272x70.data'),
	bin2c_gen.process('netascale102x82.data'),
]

if (cpu != 'm68k')
	kernel_sources += [
		files('acpi/DKACPIPCIFirmwareInterface.m', 'acpi/DKACPIPlatform.m',
			'acpi/uacpi_integration.m', 'E1000.m',),
		uacpi_sources,
	]
	freestanding_c_args += [ '-DUACPI_FORMATTED_LOGGING=1' ]
	freestanding_include_dirs += uacpi_includes
endif

if (cpu == 'amd64')
	kernel_sources += files('PS2Keyboard.m')
endif
