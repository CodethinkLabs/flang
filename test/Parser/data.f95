! RUN: %flang -fsyntax-only -verify < %s
PROGRAM datatest
  INTEGER I, J, K
  REAL X,Y,Z
  INTEGER I_ARR(10)

  DATA I / 1 /
  DATA J, K / 2*42 /

  DATA X Y / 1, 2 / ! expected-error {{expected '/'}}

  DATA X, Y / 1 2 / ! expected-error {{expected '/'}}

  DATA ! expected-error {{expected an expression}}

  DATA (I_ARR(I), I = 1,10) / 10*0 /

END PROGRAM