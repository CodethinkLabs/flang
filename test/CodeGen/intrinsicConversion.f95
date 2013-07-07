! RUN: %flang %s 2>&1 | %file_check %s
PROGRAM intrinsicconv
  INTEGER I
  REAL R
  COMPLEX C
  DOUBLE PRECISION D

  INTRINSIC int, real, dble, cmplx

  i = 0
  r = 1.0
  c = (1.0, 0.0)
  d = 1.0d0


  i = int(r)   ! CHECK: fptosi float
  continue     ! CHECK: store i32
  i = int(c)   ! CHECK: fptosi float
  continue     ! CHECK: store i32

  r = real(i)  ! CHECK: sitofp
  r = real(c)

  d = dble(i)  ! CHECK: sitofp i32
  d = dble(r)  ! CHECK: fpext float

  c = cmplx(i) ! CHECK: sitofp
  c = cmplx(r)
  c = cmplx(d) ! CHECK: fptrunc

END
