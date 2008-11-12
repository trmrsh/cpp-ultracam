/**
 * function to compute exposure time given CCD parameters
 */

void Ultracam::timer(const Ultracam::Timing& timing){

      data disp_expose_delay    / 0. /     ! in milliseconds
      data disp_inversion_delay / 110. /   ! in microseconds
      data disp_vclock_frame    / 24.4 /   ! in microseconds
      data disp_vclock_storage  / 24.4 /   ! in microseconds
      data disp_hclock          / 0.8 /    ! in microseconds
      data disp_cdstime         / 2. /     ! in microseconds
      data disp_switchtime      / 2. /     ! in microseconds

c set default frame parameters

      data lff             / .false./   ! full-frame without overscan 
      data lffover         / .false./   ! full-frame with overscan 
      data l2win           / .false./   ! 2 windowed mode
      data l4win           / .false./   ! 4 windowed mode 
      data l6win           / .false./   ! 6 windowed mode
      data ldrift          / .true./    ! drift mode
      data xbin            / 1 /        ! binning in x
      data ybin            / 1 /        ! binning in y

c set default window parameters

      data y1start, y2start, y3start        / 1, 100, 200 /
      data y1size, y2size, y3size           / 24, 24, 24 /
      data x1size, x2size, x3size           / 24, 24, 24 /
      data x1l_start, x2l_start, x3l_start  / 100, 100, 100 /
      data x1r_start, x2r_start, x3r_start  / 902, 902, 902 /

c convert timing parameters to seconds

10    expose_delay = disp_expose_delay * 1.e-3
      inversion_delay = disp_inversion_delay * 1.e-6
      vclock_frame = disp_vclock_frame * 1.e-6
      vclock_storage = disp_vclock_storage * 1.e-6
      hclock = disp_hclock * 1.e-6
      cdstime = disp_cdstime * 1.e-6
      switchtime = disp_switchtime * 1.e-6

c the time taken to read out a pixel (video) is equal to the
c correlated double sampling time plus the video chain switch time

      video = cdstime + switchtime

c prompt for timing parameters

      write(*,*)' '
      write(*,*)'timing parameters (in seconds)'
      write(*,*)'------------------------------'
      write(*,*)' '
      write(*,*)'1. exposure delay (msecs) = ',
     + disp_expose_delay
      write(*,*)'2. inversion mode delay (usecs) = ',
     + disp_inversion_delay
      write(*,*)'3. frame transfer vclock (usecs) = ',
     + disp_vclock_frame
      write(*,*)'4. storage region vclock (usecs) = ',
     + disp_vclock_storage
      write(*,*)'5. serial register hclock (usecs) = ',
     + disp_hclock
      write(*,*)'6. pixel sampling time (usecs) = ',
     + disp_cdstime
      write(*,*)'7. video chain switch time (usecs) = ',
     + disp_switchtime
      write(*,*)' '
      write(*,'(a,$)')'enter number to change (0 to continue) : '
      read(*,*)option
      if (option .eq. 1) then
         write(*,'(a,$)')
     +   'enter exposure delay in milliseconds : '
         read(*,*)disp_expose_delay
         goto 10
      else if (option .eq. 2) then
         write(*,'(a,$)')
     +   'enter inversion mode delay in microseconds : '
         read(*,*)disp_inversion_delay
         goto 10
      else if (option .eq. 3) then
         write(*,'(a,$)')
     +   'enter frame transfer vclock in microseconds : '
         read(*,*)disp_vclock_frame
         goto 10
      else if (option .eq. 4) then
         write(*,'(a,$)') 
     +   'enter storage region vclock in microseconds : '
         read(*,*)disp_vclock_storage
         goto 10
      else if (option .eq. 5) then
         write(*,'(a,$)')
     +   'enter serial register hclock in microseconds : '
         read(*,*)disp_hclock
         goto 10
      else if (option .eq. 6) then
         write(*,'(a,$)')
     +   'enter pixel sampling time in microseconds : '
         read(*,*)disp_cdstime
         goto 10
      else if (option .eq. 7) then
         write(*,'(a,$)')
     +   'enter video chain switch time in microseconds : '
         read(*,*)disp_switchtime
         goto 10
      else if (option .ne. 0) then
         goto 10
      end if

c prompt for frame parameters

20    write(*,*)' ' 
      write(*,*)'frame parameters'
      write(*,*)'----------------'
      write(*,*)' ' 
      write(*,*)'8.  full-frame mode without overscan = ',lff
      write(*,*)'9.  full-frame mode with overscan = ',lffover
      write(*,*)'10.  2 windowed mode = ',l2win 
      write(*,*)'11. 4 windowed mode = ',l4win 
      write(*,*)'12. 6 windowed mode = ',l6win 
      write(*,*)'13. drift mode = ',ldrift
      write(*,*)'14. binning in x = ',xbin
      write(*,*)'15. binning in y = ',ybin 
      write(*,*)' ' 
      write(*,'(a,$)')'enter number to change (0 to continue) : '
      read(*,*)option
      if (option .eq. 8) then
         if (lff .eq. .true.) then
            goto 20
         else
            lff = .true.
            lffover = .false.
            l2win = .false.
            l4win = .false.
            l6win = .false.
            ldrift = .false.
            goto 20
         end if
      else if (option .eq. 9) then
         if (lffover .eq. .true.) then
            goto 20
         else
            lff = .false.
            lffover = .true.
            l2win = .false.
            l4win = .false.
            l6win = .false.
            ldrift = .false.
            goto 20
         end if
      else if (option .eq. 10) then
         if (l2win .eq. .true.) then
            goto 20
         else
            lff = .false.
            lffover = .false.
            l2win = .true.
            l4win = .false.
            l6win = .false.
            ldrift = .false.
            goto 20
         end if
      else if (option .eq. 11) then
         if (l4win .eq. .true.) then
            goto 20
         else
            lff = .false.
            lffover = .false.
            l2win = .false.
            l4win = .true.
            l6win = .false.
            ldrift = .false.
            goto 20
         end if
      else if (option .eq. 12) then
         if (l6win .eq. .true.) then
            goto 20
         else
            lff = .false.
            lffover = .false.
            l2win = .false.
            l4win = .false.
            l6win = .true.
            ldrift = .false.
            goto 20
         end if
      else if (option .eq. 13) then
         if (ldrift .eq. .true.) then
            goto 20
         else
            lff = .false.
            lffover = .false.
            l2win = .false.
            l4win = .false.
            l6win = .false.
            ldrift = .true.
            goto 20
         end if
      else if (option .eq. 14) then
         write(*,'(a,$)')'enter binning in x : '
         read(*,*)xbin
         goto 20
      else if (option .eq. 15) then
         write(*,'(a,$)')'enter binning in y : '
         read(*,*)ybin
         goto 20
      else if (option .ne. 0) then
         goto 20
      end if

c prompt for window parameters

30    if (l2win .or. l4win .or. l6win .or. ldrift) then
         write(*,*)' ' 
         write(*,*)'window parameters'
         write(*,*)'-----------------'
         write(*,*)' ' 
         write(*,*)'16. window pair 1, ystart = ',y1start 
         write(*,*)'17. window pair 1, ysize = ',y1size
         write(*,*)'18. window pair 1, xsize  = ',x1size
         write(*,*)'19. window pair 1, left xstart = ',x1l_start
         write(*,*)'20. window pair 1, right xstart = ',x1r_start
         write(*,*)' '
         if (ldrift) then
            nwins = int(((1033. / y1size) + 1.)/2.)
            pshift = 1033.-(((2.*nwins)-1.)*y1size)
            write(*,*)'number of windows in pipeline = ',nwins
            write(*,*)'pshift = ',pshift
            write(*,*)' ' 
         end if
      end if
      if (l4win .or. l6win) then
         write(*,*)'21. window pair 2, ystart = ',y2start 
         write(*,*)'22. window pair 2, ysize = ',y2size
         write(*,*)'23. window pair 2, xsize  = ',x2size
         write(*,*)'24. window pair 2, left xstart = ',x2l_start
         write(*,*)'25. window pair 2, right xstart = ',x2r_start
         write(*,*)' ' 
      end if
      if (l6win) then
         write(*,*)'26. window pair 3, ystart = ',y3start 
         write(*,*)'27. window pair 3, ysize = ',y3size
         write(*,*)'28. window pair 3, xsize  = ',x3size
         write(*,*)'29. window pair 3, left xstart = ',x3l_start
         write(*,*)'30. window pair 3, right xstart = ',x3r_start
         write(*,*)' '
      end if
      if (l2win .or. l4win .or. l6win .or. ldrift) then
         write(*,'(a,$)')'enter number to change (0 to continue) : '
         read(*,*)option
         if (option .lt. 16) goto 40
      end if
      if (l2win .or. l4win .or. l6win .or. ldrift) then
         if (option .eq. 16) then
            write(*,'(a,$)')'enter ystart of window pair 1 : '
            read(*,*)y1start
            goto 30
         else if (option .eq. 17) then
            write(*,'(a,$)')'enter ysize of window pair 1 : '
            read(*,*)y1size
            goto 30
         else if (option .eq. 18) then
            write(*,'(a,$)')'enter xsize of window pair 1 : '
            read(*,*)x1size
            goto 30
         else if (option .eq. 19) then
            write(*,'(a,$)')'enter xstart of left window of pair 1 : '
            read(*,*)x1l_start
            goto 30
         else if (option .eq. 20) then
            write(*,'(a,$)')'enter xstart of right window of pair 1 : '
            read(*,*)x1r_start
            goto 30
         end if
      end if
      if (l4win .or. l6win) then
         if (option .eq. 21) then
            write(*,'(a,$)')'enter ystart of window pair 2 : '
            read(*,*)y2start
            goto 30
         else if (option .eq. 22) then
            write(*,'(a,$)')'enter ysize of window pair 2 : '
            read(*,*)y2size
            goto 30
         else if (option .eq. 23) then
            write(*,'(a,$)')'enter xsize of window pair 2 : '
            read(*,*)x2size
            goto 30
         else if (option .eq. 24) then
            write(*,'(a,$)')'enter xstart of left window of pair 2 : '
            read(*,*)x2l_start
            goto 30
         else if (option .eq. 25) then
            write(*,'(a,$)')'enter xstart of right window of pair 2 : '
            read(*,*)x2r_start
            goto 30
         end if
      end if
      if (l6win) then
         if (option .eq. 26) then
            write(*,'(a,$)')'enter ystart of window pair 3 : '
            read(*,*)y3start
            goto 30
         else if (option .eq. 27) then
            write(*,'(a,$)')'enter ysize of window pair 3 : '
            read(*,*)y3size
            goto 30
         else if (option .eq. 28) then
            write(*,'(a,$)')'enter xsize of window pair 3 : '
            read(*,*)x3size
            goto 30
         else if (option .eq. 29) then
            write(*,'(a,$)')'enter xstart of left window of pair 3 : '
            read(*,*)x3l_start
            goto 30
         else if (option .eq. 30) then
            write(*,'(a,$)')'enter xstart of right window of pair 3 : '
            read(*,*)x3r_start
            goto 30
         end if
      end if

c calculate exposure time and dead-time for full-frame mode without overscan

40    if (lff) then

c clear chip by vclocking the image and storage areas

         clear_time = (1033. + 1027.) * vclock_frame

c move image into storage area (note that this uses what we call the
c "frame transfer" clocks. There is a separate vclock, which I have
c called vclock_storage, which is used just for the line reads (see
c below)

         frame_transfer = 1033.*vclock_frame

c calculate how long it takes to shift one row into the serial register,
c shift along the serial register (including the diffshift), and 
c then read out the data

         line_read = (vclock_storage*ybin) + 
     +               (536.*hclock) + 
     +               (((512./xbin)+2.)*video)

c now multiply this by the number of rows to obtain the total readout time

         readout = (1024./ybin)*line_read

c the total time taken to read out one exposure is the sum of the 
c following

         cycle_time = inversion_delay + expose_delay + clear_time + 
     +                frame_transfer + readout
         frame_rate = 1. / cycle_time
         exposure_time = expose_delay
         dead_time = cycle_time - exposure_time
         write(*,*)' '
         write(*,*)'results (in clear mode)'
         write(*,*)'-----------------------'
         write(*,*)' '
         write(*,*)'inversion delay = ',inversion_delay
         write(*,*)'exposure delay = ',expose_delay
         write(*,*)'clear time = ',clear_time
         write(*,*)'frame transfer = ',frame_transfer
         write(*,*)'line read = ',line_read
         write(*,*)'readout time = ',readout
         write(*,*)' '
         write(*,*)'cycle time = ',cycle_time
         write(*,*)'frame rate = ',frame_rate
         write(*,*)'exposure time = ',exposure_time
         write(*,*)'dead time = ',dead_time
         write(*,*)' '

c output results for no-clear mode as well

         cycle_time = inversion_delay + expose_delay + 
     +                frame_transfer + readout
         frame_rate = 1. / cycle_time
         exposure_time = cycle_time - frame_transfer
         dead_time = cycle_time - exposure_time

         write(*,*)'results (in no-clear mode)'
         write(*,*)'--------------------------'
         write(*,*)' '
         write(*,*)'cycle time = ',cycle_time
         write(*,*)'frame rate = ',frame_rate
         write(*,*)'exposure time = ',exposure_time
         write(*,*)'dead time = ',dead_time
         write(*,*)' '

c calculate exposure time and dead-time for full-frame mode with overscan

      else if (lffover) then

c clear chip by vclocking the image and storage areas

         clear_time = (1033. + 1032.) * vclock_frame

c move image into storage area (note that this uses what we call the
c "frame transfer" clocks. There is a separate vclock, which I have
c called vclock_storage, which is used just for the line reads (see
c below)

         frame_transfer = 1033.*vclock_frame

c calculate how long it takes to shift one row into the serial register,
c shift along the serial register (including the diffshift), and 
c then read out the data

         line_read = (vclock_storage*ybin) + 
     +               (540.*hclock) + 
     +               (((540./xbin)+2.)*video)

c now multiply this by the number of rows to obtain the total readout time

         readout = (1032./ybin)*line_read

c the total time taken to read out one exposure is the sum of the 
c following

         cycle_time = inversion_delay + expose_delay + clear_time +
     +                frame_transfer + readout
         frame_rate = 1. / cycle_time
         exposure_time = cycle_time - frame_transfer
         dead_time = cycle_time - exposure_time

c output results (note that the full frame with overscan mode is 
c not available with the no-clear option)

         write(*,*)' '
         write(*,*)'results'
         write(*,*)'-------'
         write(*,*)' '
         write(*,*)'inversion delay = ',inversion_delay
         write(*,*)'exposure delay = ',expose_delay
         write(*,*)'clear time = ',clear_time
         write(*,*)'frame transfer = ',frame_transfer
         write(*,*)'line read = ',line_read
         write(*,*)'readout time = ',readout
         write(*,*)' '
         write(*,*)'cycle time = ',cycle_time
         write(*,*)'frame rate = ',frame_rate
         write(*,*)'exposure time = ',exposure_time
         write(*,*)'dead time = ',dead_time
         write(*,*)' '

c calculate exposure time and dead-time for 2 windowed mode

      else if (l2win) then

c move window into storage area (note that this uses what we call the
c "frame transfer" clocks. There is a separate vclock, which I have
c called vclock_storage, which is used just for the line reads (see
c below)

         frame_transfer = 1033.*vclock_frame

c calculate the yshift, which places the window adjacent to the
c serial register

         yshift_1 = (y1start-1.) * vclock_storage

c calculate the diffshift, which aligns the windows in the serial register
c so that they can be read out simultaneously

         diffshift_1 = abs((x1l_start-1.) - (1024.-x1r_start-x1size+1.))
         write(*,*)' '
         write(*,*)'diffshift_1 = ',diffshift_1

c calculate how long it takes to shift one row into the serial register,
c shift along the serial register (including the diffshift), and 
c then read out the data

         line_read_1 = (vclock_storage*ybin) + 
     +                 ((536.+diffshift_1)*hclock) + 
     +                 (((x1size/xbin)+2.)*video)

c now multiply this by the number of rows to obtain the total readout time

         readout_1 = (y1size/ybin)*line_read_1

c the total time taken to read out one exposure is the sum of the 
c following

         cycle_time = inversion_delay + expose_delay + 
     +                frame_transfer + yshift_1 + readout_1
         frame_rate = 1. / cycle_time
         exposure_time = cycle_time - frame_transfer
         dead_time = cycle_time - exposure_time
         write(*,*)' '
         write(*,*)'results'
         write(*,*)'-------'
         write(*,*)' '
         write(*,*)'inversion delay = ',inversion_delay
         write(*,*)'exposure delay = ',expose_delay
         write(*,*)'frame transfer = ',frame_transfer
         write(*,*)'line read = ',line_read_1
         write(*,*)'readout time = ',readout_1
         write(*,*)' '
         write(*,*)'cycle time = ',cycle_time
         write(*,*)'frame rate = ',frame_rate
         write(*,*)'exposure time = ',exposure_time
         write(*,*)'dead time = ',dead_time
         write(*,*)' '

c calculate exposure time and dead-time for 4 windowed mode

      else if (l4win) then

c move window into storage area (note that this uses what we call the
c "frame transfer" clocks. There is a separate vclock, which I have
c called vclock_storage, which is used just for the line reads (see
c below)

         frame_transfer = 1033.*vclock_frame

c calculate the yshifts, which places the windows adjacent to the
c serial register

         yshift_1 = (y1start-1.) * vclock_storage
         yshift_2 = (y2start-y1start-y1size-1.) * vclock_storage

c calculate the diffshift, which aligns the windows in the serial register
c so that they can be read out simultaneously

         diffshift_1 = abs((x1l_start-1.) - (1024.-x1r_start-x1size+1.))
         diffshift_2 = abs((x2l_start-1.) - (1024.-x2r_start-x2size+1.))
         write(*,*)' '
         write(*,*)'diffshift_1 = ',diffshift_1
         write(*,*)'diffshift_2 = ',diffshift_2

c calculate how long it takes to shift one row into the serial register,
c shift along the serial register (including the diffshift), and 
c then read out the data

         line_read_1 = (vclock_storage*ybin) + 
     +               ((536.+diffshift_1)*hclock) + 
     +               (((x1size/xbin)+2.)*video)
         line_read_2 = (vclock_storage*ybin) + 
     +               ((536.+diffshift_2)*hclock) + 
     +               (((x2size/xbin)+2.)*video)

c now multiply this by the number of rows to obtain the total readout time

         readout_1 = (y1size/ybin)*line_read_1
         readout_2 = (y2size/ybin)*line_read_2

c the total time taken to read out one exposure is the sum of the 
c following

         cycle_time = inversion_delay + expose_delay + 
     +                frame_transfer + yshift_1 + yshift_2 + 
     +                readout_1 + readout_2
         frame_rate = 1. / cycle_time
         exposure_time = cycle_time - frame_transfer
         dead_time = cycle_time - exposure_time
         write(*,*)' '
         write(*,*)'results'
         write(*,*)'-------'
         write(*,*)' '
         write(*,*)'inversion delay = ',inversion_delay
         write(*,*)'exposure delay = ',expose_delay
         write(*,*)'frame transfer = ',frame_transfer
         write(*,*)'line read = ',line_read_1+line_read_2
         write(*,*)'readout time = ',readout_1+readout_2
         write(*,*)' '
         write(*,*)'cycle time = ',cycle_time
         write(*,*)'frame rate = ',frame_rate
         write(*,*)'exposure time = ',exposure_time
         write(*,*)'dead time = ',dead_time
         write(*,*)' '

c calculate exposure time and dead-time for 6 windowed mode

      else if (l6win) then

c move window into storage area (note that this uses what we call the
c "frame transfer" clocks. There is a separate vclock, which I have
c called vclock_storage, which is used just for the line reads (see
c below)

         frame_transfer = 1033.*vclock_frame

c calculate the yshifts, which places the windows adjacent to the
c serial register

         yshift_1 = (y1start-1.) * vclock_storage
         yshift_2 = (y2start-y1start-y1size-1.) * vclock_storage
         yshift_3 = (y3start-y2start-y1start-y2size-y1size-1.) *
     +              vclock_storage

c calculate the diffshift, which aligns the windows in the serial register
c so that they can be read out simultaneously

         diffshift_1 = abs((x1l_start-1.) - (1024.-x1r_start-x1size+1.))
         diffshift_2 = abs((x2l_start-1.) - (1024.-x2r_start-x2size+1.))
         diffshift_3 = abs((x3l_start-1.) - (1024.-x3r_start-x3size+1.))
         write(*,*)' '
         write(*,*)'diffshift_1 = ',diffshift_1
         write(*,*)'diffshift_2 = ',diffshift_2
         write(*,*)'diffshift_3 = ',diffshift_3

c calculate how long it takes to shift one row into the serial register,
c shift along the serial register (including the diffshift), and 
c then read out the data

         line_read_1 = (vclock_storage*ybin) + 
     +               ((536.+diffshift_1)*hclock) + 
     +               (((x1size/xbin)+2.)*video)
         line_read_2 = (vclock_storage*ybin) + 
     +               ((536.+diffshift_2)*hclock) + 
     +               (((x2size/xbin)+2.)*video)
         line_read_3 = (vclock_storage*ybin) + 
     +               ((536.+diffshift_3)*hclock) + 
     +               (((x3size/xbin)+2.)*video)

c now multiply this by the number of rows to obtain the total readout time

         readout_1 = (y1size/ybin)*line_read_1
         readout_2 = (y2size/ybin)*line_read_2
         readout_3 = (y3size/ybin)*line_read_3

c the total time taken to read out one exposure is the sum of the 
c following

         cycle_time = inversion_delay + expose_delay + 
     +                frame_transfer + yshift_1 + yshift_2 + 
     +                yshift_3 + readout_1 + readout_2 + 
     +                readout_3
         frame_rate = 1. / cycle_time
         exposure_time = cycle_time - frame_transfer
         dead_time = cycle_time - exposure_time
         write(*,*)' '
         write(*,*)'results'
         write(*,*)'-------'
         write(*,*)' '
         write(*,*)'inversion delay = ',inversion_delay
         write(*,*)'exposure delay = ',expose_delay
         write(*,*)'frame transfer = ',frame_transfer
         write(*,*)'line read = ',
     +   line_read_1+line_read_2+line_read_3
         write(*,*)'readout time = ',
     +   readout_1+readout_2+readout_3
         write(*,*)' '
         write(*,*)'cycle time = ',cycle_time
         write(*,*)'frame rate = ',frame_rate
         write(*,*)'exposure time = ',exposure_time
         write(*,*)'dead time = ',dead_time
         write(*,*)' '

c calculate exposure time and dead-time for drift mode

      else if (ldrift) then

c move window to top of storage area (note that this uses what we call
c the "frame transfer" clocks. There is a separate vclock, which I have
c called vclock_storage, which is used just for the line reads (see
c below)

         frame_transfer = (y1size + y1start-1.)*vclock_frame

c calculate the diffshift, which aligns the windows in the serial register
c so that they can be read out simultaneously

         diffshift_1 = abs((x1l_start-1.) - (1024.-x1r_start-x1size+1.))
         write(*,*)' '
         write(*,*)'diffshift_1 = ',diffshift_1

c calculate how long it takes to shift one row into the serial register,
c shift along the serial register (including the diffshift), and 
c then read out the data

         line_read_1 = (vclock_storage*ybin) + 
     +               ((536.+diffshift_1)*hclock) + 
     +               (((x1size/xbin)+2.)*video)

c now multiply this by the number of rows to obtain the total readout time

         readout_1 = (y1size/ybin)*line_read_1

c the total time taken to read out one exposure is the sum of the 
c following

         cycle_time = inversion_delay + (pshift*vclock_storage) + 
     +                expose_delay + frame_transfer + readout_1
         frame_rate = 1. / cycle_time
         exposure_time = cycle_time - frame_transfer
         dead_time = cycle_time - exposure_time
         write(*,*)' '
         write(*,*)'results'
         write(*,*)'-------'
         write(*,*)' '
         write(*,*)'inversion delay = ',inversion_delay
         write(*,*)'exposure delay = ',expose_delay
         write(*,*)'frame transfer = ',frame_transfer
         write(*,*)'line read = ',line_read_1
         write(*,*)'readout time = ',readout_1
         write(*,*)' '
         write(*,*)'cycle time = ',cycle_time
         write(*,*)'frame rate = ',frame_rate
         write(*,*)'exposure time = ',exposure_time
         write(*,*)'dead time = ',dead_time
         write(*,*)' '
      end if

c prompt for another calculation 

50    write(*,'(a,$)')'another calculation [y/n] ? : '
      read(*,'(a)')recalc
      if (recalc .eq. 'n') then
         goto 99
      else if (recalc .eq. 'y') then
         goto 10
      else
         goto 50
      end if
99    end
