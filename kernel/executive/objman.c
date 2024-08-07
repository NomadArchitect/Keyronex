#include <stdint.h>

#include "kdk/file.h"
#include "kdk/kern.h"
#include "kdk/kmem.h"
#include "kdk/libkern.h"
#include "kdk/object.h"
#include "kdk/vfs.h"
#include "object.h"

/* process.c */
void ex_thread_free(void *ptr);
void ex_proc_free(void *ptr);

/*!
 * the inactive procedure should return true if the object was deleted, false
 * if it was not, and a deferred refcount drop must now be carried out.
 */

struct type_object {
	/*! all objects of this type */
	LIST_HEAD(, object_header) objects;
	/*! count of objects of this type */
	uint32_t count;
	/*! guards objects */
	kmutex_t mutex;
	/*! free the object */
	void (*free)(void *obj);
};

struct type_object_with_header {
	struct object_header header;
	struct type_object object;
};

static struct type_object_with_header types[255];
obj_class_t process_class, thread_class, file_class, anon_class;

static inline struct object_header *
header_from_object(void *object)
{
	return (void *)(((char *)object) - sizeof(struct object_header));
}

static inline void *
object_from_header(void *header)
{
	return (void *)(((char *)header) + sizeof(struct object_header));
}

static void
new_common(struct object_header *hdr, struct type_object *type,
    const char *name, size_t size)
{
	kassert(splget() < kIPLDPC);
	obj_retain(type);
	ke_wait(&type->mutex, "new_common:type->mutex", false, false, -1);
#if 0
	LIST_INSERT_HEAD(&type->objects, hdr, list_link);
#endif
	type->count++;
	hdr->name = name == NULL ? NULL : strdup(name);
	hdr->size = size;
	hdr->refcount = 1;
	ke_mutex_release(&type->mutex);
}

obj_class_t
obj_new_type(const char *name, obj_free_fn_t free)
{
	obj_class_t type_index = (uint8_t)-1;
	struct type_object *type;

	for (int i = 0; i < 255; i++)
		if (types[i].header.class == (uint8_t)-1) {
			type_index = i;
			break;
		}

	kassert(type_index != (uint8_t)-1);

	types[type_index].header.refcount = 1;
	/* the class of Type is Type */
	types[type_index].header.class = 0;

	new_common(&types[type_index].header, &types[0].object, name,
	    sizeof(struct type_object));
	type = &types[type_index].object;

	type->free = free;

	kprintf("obj_new_type: new type %s defined (id %d)\n", name,
	    type_index);

	return type_index;
}

void
obj_init(void)
{
	for (int i = 0; i < 255; i++) {
		ke_mutex_init(&types[i].object.mutex);
		LIST_INIT(&types[i].object.objects);
		types[i].header.class = (uint8_t)-1;
	}

	obj_new_type("Type", NULL);
	anon_class = obj_new_type("Anonymous", NULL);
	process_class = obj_new_type("process", ex_proc_free);
	thread_class = obj_new_type("thread", ex_thread_free);
	file_class = obj_new_type("file", ex_file_free);

	kassert(process_class == 2);
	kassert(thread_class == 3);
}

int
obj_new(void *ptr_out, obj_class_t klass, size_t size, const char *name)
{
	struct type_object *type_obj;
	struct object_header *hdr;

	type_obj = &types[klass].object;
	obj_retain((type_obj));

	hdr = kmem_alloc(sizeof(struct object_header) + size);
	hdr->class = klass;
	new_common(hdr, &types[klass].object, name, size);

	*(void **)ptr_out = object_from_header(hdr);

	return 0;
}

void *
obj_retain(void *object)
{
	struct object_header *hdr = header_from_object(object);
	uint32_t count = __atomic_fetch_add(&hdr->refcount, 1,
	    __ATOMIC_RELAXED);
	kassert(count != 0 && count < 0xfffffff0);
	return object;
}

/*! @brief Retains an object only if its refcount is not 0. */
void *
obj_tryretain_rcu(void *object)
{
	struct object_header *hdr = header_from_object(object);
	while (true) {
		uint32_t count = __atomic_load_n(&hdr->refcount,
		    __ATOMIC_RELAXED);
		if (count == 0) {
			return NULL;
		}
		if (__atomic_compare_exchange_n(&hdr->refcount, &count,
			count + 1, false, __ATOMIC_ACQ_REL, __ATOMIC_RELAXED)) {
			return object;
		}
	}
}

/*! @brief Release a pointer to an object. */
void
obj_release(void *object)
{
	struct object_header *hdr = header_from_object(object);
	uint32_t count = __atomic_fetch_sub(&hdr->refcount, 1,
	    __ATOMIC_ACQ_REL);

	if (count == 1) {
		struct type_object *type = &types[hdr->class].object;
		type->free(object);
	}
}

const char *
obj_name(void *obj)
{
	struct object_header *hdr = header_from_object(obj);
	return hdr->name;
}

char **
obj_name_ptr(void *obj)
{
	struct object_header *hdr = header_from_object(obj);
	return &hdr->name;
}

void
obj_dump(void)
{
	kprintf(" -- Objects --\n");
	for (int i = 0; i < 255; i++) {
		struct type_object_with_header *type = &types[i];
		struct object_header *header;

		if (type->header.class == (uint8_t)-1)
			continue;

		kprintf("Type: %s\n", type->header.name);

		LIST_FOREACH (header, &type->object.objects, list_link) {
			kprintf(" Object <%s> (%p) RC %u\n",
			    header->name == NULL ? "?" : header->name,
			    object_from_header(header), header->refcount);
		}
	}
}
