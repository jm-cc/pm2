!
! PM2: Parallel Multithreaded Machine
! Copyright (C) 2006 "the PM2 team" (see AUTHORS file)
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

  external marcel_create
  external marcel_join
  external marcel_self
  external marcel_yield
  external writer

  integer, parameter :: nb = 3
  integer i
  integer (kind=8) :: pid(nb)

  call marcel_init()

  do i = 1 , nb
     call marcel_create(pid(i), writer, i) ! a tester sur d'autres architectures et compilateurs
  end do

  call marcel_yield()

   do i = 1 , nb
      call marcel_join (pid(i))
   end do

  call marcel_end()

end program launch


subroutine writer(arg)

  integer arg
  integer (kind=8) :: self

  call marcel_self(self)
  write (*,*) "marcel says I'm", self, "and my arg say I'm thread number", arg

end subroutine writer
