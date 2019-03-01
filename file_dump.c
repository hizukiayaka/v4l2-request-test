/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "v4l2-request-test.h"

static int32_t dump_count = -1;

static int map_imported_buffer(int *import_fds, unsigned int import_fds_count,
			       unsigned int *offsets, unsigned int *sizes,
			       struct file_buffer *buffer)
{
	unsigned int i;

	memset(buffer, 0, sizeof(*buffer));

	for (i = 0; i < import_fds_count; i++) {
		void *data = NULL;

		data = mmap(NULL, sizes[i] - offsets[i], PROT_READ, MAP_SHARED,
			    import_fds[i], offsets[i]);
		if (data == MAP_FAILED) {
			fprintf(stderr, "Unable to mmap plane %d buffer: %s\n",
				i, strerror(errno));
			return -1;
		}

		buffer->fds[i] = import_fds[i];
		buffer->data[i] = data;
		buffer->size[i] = sizes[i];
		buffer->offsets[i] = offsets[i];
	}

	return 0;
}

static int destroy_buffer(struct file_buffer *buffer)
{
	if (buffer == NULL)
		return -1;

	for (int i = 0; i < 4; i++) {
		if (NULL == buffer->data[i])
			continue;

		munmap(buffer->data[i], buffer->size[i]);
	}
	return 0;
}

int file_dump_start(struct format_description *format,
		    struct video_buffer *video_buffers, unsigned int count,
		    struct file_buffer **buffers)
{
	struct video_buffer *video_buffer;
	struct file_buffer *buffer;
	unsigned int export_fds_count;
	int rc;

	*buffers = calloc(count, sizeof(struct file_buffer));

	for (int i = 0; i < count; i++) {
		buffer = &((*buffers)[i]);
		video_buffer = &video_buffers[i];
		export_fds_count = video_buffer->destination_buffers_count;

		rc = map_imported_buffer(video_buffer->export_fds,
					 export_fds_count,
					 video_buffer->destination_offsets,
					 video_buffer->destination_sizes,
					 buffer);
		if (rc < 0) {
			fprintf(stderr,
				"Unable to map v4l2 buffer, index: %d\n", i);
			return -1;
		}

		buffer->planes_count = format->planes_count;
	}
	dump_count = 0;

	return 0;
}

int file_engine_stop(struct file_buffer *buffers, int count)
{
	struct file_buffer *buffer;
	unsigned int i;
	int rc;

	if (buffers == NULL)
		return -1;

	for (i = 0; i < count; i++) {
		buffer = &buffers[i];

		rc = destroy_buffer(buffer);
		if (rc < 0) {
			fprintf(stderr, "Unable to destroy buffer %d\n", i);
			return -1;
		}
	}
	free(buffers);

	dump_count = -1;

	return 0;
}

int file_dump_image(unsigned int index, struct file_buffer *buffers)
{
	struct file_buffer *buffer;
	FILE *output_file = NULL;
	unsigned int i;
	char file_name[40] = { 0, };

	if (buffers == NULL)
		return -1;

	buffer = &buffers[index];

	snprintf(file_name, sizeof(file_name), "out/dump_%d", dump_count);

	output_file = fopen(file_name, "w+b");
	if (NULL == output_file) {
		fprintf(stderr, "can't open file %s\n", file_name);
		return -1;
	}
	dump_count++;

	for (i = 0; i < buffer->planes_count; i++) {
		if (!buffer->data[i]) {
			fprintf(stderr, "no data at plane %d\n", i);
			continue;
		}
		fwrite(buffer->data[i], buffer->size[i], 1, output_file);
	}

#if 0
	rc = page_flip(drm_fd, setup->crtc_id, setup->plane_id,
		       &setup->properties_ids, buffer->framebuffer_id);
	if (rc < 0) {
		fprintf(stderr, "Unable to flip page to framebuffer %d\n",
			buffer->framebuffer_id);
		return -1;
	}
#endif
	fclose(output_file);
	output_file = NULL;

	return 0;
}
