project('m68k-virt-loader')

add_languages('c')

freestanding_c_args = [
	'-fno-strict-aliasing',
	'-fwrapv',
	'-fno-delete-null-pointer-checks',
	'-fno-omit-frame-pointer',
	'-ffreestanding',
	'-fno-stack-protector',
	'-fno-stack-check',
	'-fno-pie',
	'-fno-pic',
]

loader_virt_m68k = executable('keyronex-loader-m68k-virt', 'entry.c',
	'locore.S',
	c_args: [ freestanding_c_args, '-mcpu=68040', ],
	link_args: [ '-nostdlib', '-static', '-Ttext=0x1000', '-mcpu=68040', '-lgcc',
		'-Wl,-T' + meson.current_source_dir() + '/linker.ld'
	]
)
