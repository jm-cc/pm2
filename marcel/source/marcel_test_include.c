
/* Ce fichier sert à vérifier que "marcel.h" inclut bien tout les .h
 * générés...
 *
 * ...sauf ceux que l'on ne désire pas à lister avec des defines
 * explicitement.
 */
#include "marcel.h"

/* Fichier .h non inclus (mettre le #if à 0 pour les voir apparaître
 * lors de la compilation)
 */
#ifndef MA__LWPS
#define ASM_LINUX_SPINLOCK___COMPILER_H
#define ASM_LINUX_SPINLOCK___MACROS_H
#define ASM_LINUX_SPINLOCK___TYPES_H
#define ASM_LINUX_SPINLOCK___STRUCTURES_H
#define ASM_LINUX_SPINLOCK___VARIABLES_H
#define ASM_LINUX_SPINLOCK___FUNCTIONS_H
#define ASM_LINUX_SPINLOCK___DECLARATIONS_H
#define ASM_LINUX_SPINLOCK___INLINE_H
#define ASM_LINUX_SPINLOCK___MARCEL_COMPILER_H
#define ASM_LINUX_SPINLOCK___MARCEL_MACROS_H
#define ASM_LINUX_SPINLOCK___MARCEL_TYPES_H
#define ASM_LINUX_SPINLOCK___MARCEL_STRUCTURES_H
#define ASM_LINUX_SPINLOCK___MARCEL_VARIABLES_H
#define ASM_LINUX_SPINLOCK___MARCEL_FUNCTIONS_H
#define ASM_LINUX_SPINLOCK___MARCEL_DECLARATIONS_H
#define ASM_LINUX_SPINLOCK___MARCEL_INLINE_H
#endif

#if 1
#define SCHEDULERMARCEL_MARCEL_SCHED___SCHED_MARCEL_INLINE_H
#define SCHEDULERMARCEL_MARCEL_SCHED___SCHED_MARCEL_FUNCTIONS_H
#define MARCEL_SCHED_GENERIC___SCHED_MARCEL_INLINE_H
#define MARCEL_SCHED_GENERIC___SCHED_MARCEL_FUNCTIONS_H
#define MARCEL_UTILS___STRINGIFICATION_H
#endif

#define __SPLIT_SECTION_NOT_YET_INCLUDED__
#define __SPLIT_SECTION_INCLUDE_ALL
#include "asm/marcel-master___all-sections.h"
#include "marcel-master___all-sections.h"
#include "scheduler/marcel-master___all-sections.h"

#ifdef __HAS_SPLIT_SECTION_NOT_YET_INCLUDED__
#error Please, correct previous includes
#endif
