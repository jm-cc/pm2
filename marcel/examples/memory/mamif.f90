!
! PM2: Parallel Multithreaded Machine
! Copyright (C) 2008 "the PM2 team" (see AUTHORS file)
!
! This program is free software; you can redistribute it and/or modify
! it under the terms of the GNU General Public License as published by
! the Free Software Foundation; either version 2 of the License, or (at
! your option) any later version.
!
! This program is distributed in the hope that it will be useful, but
! WITHOUT ANY WARRANTY; without even the implied warranty of
! MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
! General Public License for more details.
!

program launch

  integer node, ierr
  integer memory_manager
  integer buffer(100)

  call marcel_init()
  call marcel_memory_init(memory_manager)

  call marcel_memory_register(memory_manager, buffer, 100, 0, ierr)
  call marcel_memory_locate(memory_manager, buffer, node, ierr)

  print *,'node=',node

  buffer(10) = 42

  print *,'buffer(10)=',buffer(10)

  call marcel_memory_print(memory_manager)
  call marcel_memory_unregister(memory_manager, buffer, ierr)
  call marcel_memory_exit(memory_manager)
  call marcel_end()

end program launch
