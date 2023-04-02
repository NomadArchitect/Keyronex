/*
 * Copyright (c) 2023 NetaScale Object Solutions.
 * Created on Sun Apr 02 2023.
 */

#include "9pfs_reg.h"
#include "kdk/kernel.h"
#include "kdk/kmem.h"
#include "kdk/libkern.h"

struct ninep_buf *
ninep_buf_alloc(const char *fmt)
{
	struct ninep_buf *buf;
	size_t size = sizeof(struct ninep_hdr);

	while (*fmt != '\0') {
		switch (*fmt++) {
		case 'Q':
			size += sizeof(struct ninep_qid);
			break;

		case 'l':
			size += sizeof(uint64_t);
			break;

		case 'F':
		case 'd':
			size += sizeof(uint32_t);
			break;

		case 'h':
			size += sizeof(uint16_t);
			break;

		case 'S': {
			/* we need a length to work with */
			int len;
			kassert(isdigit(*fmt));
			size += sizeof(uint16_t);
			len = atoi(fmt);
			kassert(len > 0);
			size += len;
			while (isdigit(*fmt))
				fmt++;
			break;
		}

		default: {
			kfatal("Bad 9p format\n");
		}
		}
	}

	struct ninep_version_hdr *fanny = NULL;
	(void)fanny;

	buf = kmem_alloc(sizeof(*buf));
	buf->bufsize = size;
	buf->data = kmem_alloc(size);
	buf->offset = 0;

	return buf;
}

void
ninep_buf_free(struct ninep_buf *buf)
{
	kmem_free(buf->data, buf->bufsize);
	kmem_free(buf, sizeof(*buf));
}

void
ninep_buf_addu16(struct ninep_buf *buf, uint16_t num)
{
	io_off_t newoff = buf->offset + sizeof(uint16_t);

	kassert(newoff <= buf->bufsize - sizeof(struct ninep_hdr));

	*((uint16_t *)&buf->data->data[buf->offset]) = num;
	buf->offset += sizeof(uint16_t);
}

void
ninep_buf_addu32(struct ninep_buf *buf, uint32_t num)
{
	io_off_t newoff = buf->offset + sizeof(uint32_t);

	kassert(newoff <= buf->bufsize - sizeof(struct ninep_hdr));

	*((uint32_t *)&buf->data->data[buf->offset]) = num;
	buf->offset += sizeof(uint32_t);
}

void
ninep_buf_addu64(struct ninep_buf *buf, uint64_t num)
{
	io_off_t newoff = buf->offset + sizeof(uint64_t);

	kassert(newoff <= buf->bufsize - sizeof(struct ninep_hdr));

	*((uint64_t *)&buf->data->data[buf->offset]) = num;
	buf->offset += sizeof(uint64_t);
}

void
ninep_buf_addstr(struct ninep_buf *buf, const char *str)
{
	size_t slen = strlen(str);
	io_off_t newoff = buf->offset + sizeof(uint16_t) + slen;

	kassert(newoff <= buf->bufsize + sizeof(struct ninep_hdr));

	*((uint16_t *)&buf->data->data[buf->offset]) = slen;
	buf->offset += sizeof(uint16_t);
	memcpy(&buf->data->data[buf->offset], str, slen);

	buf->offset = newoff;
}

void
ninep_buf_close(struct ninep_buf *buf)
{
	buf->data->size = buf->offset + sizeof(struct ninep_hdr);
}

int
ninep_buf_getu16(struct ninep_buf *buf, uint16_t *num_out)
{
	*num_out = *((uint16_t *)&buf->data->data[buf->offset]);
	buf->offset += sizeof(uint16_t);
	return 0;
}

int
ninep_buf_getu32(struct ninep_buf *buf, uint32_t *num_out)
{
	if (num_out != NULL)
		*num_out = *((uint32_t *)&buf->data->data[buf->offset]);
	buf->offset += sizeof(uint32_t);
	return 0;
}

int
ninep_buf_getu64(struct ninep_buf *buf, uint64_t *num_out)
{
	if (num_out != NULL)
		*num_out = *((uint64_t *)&buf->data->data[buf->offset]);
	buf->offset += sizeof(uint64_t);
	return 0;
}

int
ninep_buf_getstr(struct ninep_buf *buf, char **str_out)
{
	size_t slen;
	char *str;

	slen = *((uint16_t *)&buf->data->data[buf->offset]);
	buf->offset += 2;

	str = kmem_alloc(slen + 1);
	memcpy(str, &buf->data->data[buf->offset], slen);

	str[slen] = '\0';
	*str_out = str;

	return 0;
}

int
ninep_buf_getqid(struct ninep_buf *buf, struct ninep_qid *qid_out)
{
	if (qid_out == NULL) {
		buf->offset += sizeof(struct ninep_qid);
		return 0;
	}

	qid_out->type = *((uint8_t *)&buf->data->data[buf->offset]);
	buf->offset += 1;

	qid_out->version = *((uint32_t *)&buf->data->data[buf->offset]);
	buf->offset += sizeof(uint32_t);

	qid_out->path = *((uint64_t *)&buf->data->data[buf->offset]);
	buf->offset += sizeof(uint64_t);

	return 0;
}