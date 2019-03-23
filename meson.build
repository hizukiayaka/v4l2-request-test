project('v4l2-request-test', 'c')

dep_libdrm = dependency('libdrm', version: '>= 2.4.68')
dep_libdrm_headers = dep_libdrm.partial_dependency(compile_args: true)

src = ['v4l2-request-test.c', 'v4l2.c', 'presets.c', 'drm.c', 'file_dump.c']

executable('v4l2-request-test',
	sources: src,
	include_directories: include_directories('include'),
	dependencies: dep_libdrm,
)
