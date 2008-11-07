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
  integer memory_manager(4)
  integer buffer(100)

  call marcel_init()
  call marcel_memory_init(memory_manager)

  call marcel_memory_malloc(memory_manager, 100, buffer)
  call marcel_memory_locate(memory_manager, buffer, node, ierr)

  print *,'node=',node

  call marcel_memory_print(memory_manager)

end program launch
