
#ifndef ISOADDR_ATTR_IS_DEF
#define ISOADDR_ATTR_IS_DEF

#define isoaddr_attr_set_status(attr, _status) (attr)->status = _status

#define isoaddr_attr_set_protocol(attr, _protocol) (attr)->protocol = _protocol

#define isoaddr_attr_set_alignment(attr, _align) (attr)->align = _align

#define isoaddr_attr_set_atomic(attr) (attr)->atomic = 1;

#define isoaddr_attr_set_non_atomic(attr) (attr)->atomic = 0;

#define isoaddr_attr_get_status(attr) ((attr)->status)

#define isoaddr_attr_get_protocol(attr) ((attr)->protocol)

#define isoaddr_attr_get_alignment(attr) ((attr)->align)

#define isoaddr_attr_atomic(attr) ((attr)->atomic)

#endif


