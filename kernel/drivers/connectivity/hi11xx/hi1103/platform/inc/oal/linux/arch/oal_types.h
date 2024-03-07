

#ifndef __OAL_LINUX_TYPES_H__
#define __OAL_LINUX_TYPES_H__

/* �����������Ͷ��� */
typedef signed char oal_int8;          /* ����ǰ׺:c */
typedef signed short oal_int16;        /* ����ǰ׺:s */
typedef signed int oal_int32;          /* ����ǰ׺:l */
typedef signed long long oal_int64;    /* ����ǰ׺:ll */
typedef unsigned char oal_uint8;       /* ����ǰ׺:uc */
typedef unsigned short oal_uint16;     /* ����ǰ׺:us */
typedef unsigned int oal_uint32;       /* ����ǰ׺:ul */
typedef unsigned long long oal_uint64; /* ����ǰ׺:ull */
typedef void oal_void;

/* �궨�� */
#ifdef INLINE_TO_FORCEINLINE
#define OAL_INLINE __forceinline
#else
#define OAL_INLINE inline
#endif

#define OAL_STATIC static

#if defined(_PRE_PC_LINT)
#define OAL_VOLATILE
#else
#define OAL_VOLATILE volatile
#endif

#endif /* end of oal_types.h */