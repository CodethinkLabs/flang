! RUN: %flang -verify %s 2>&1 | %file_check %s
PROGRAM expressions
  REAL X,Y,Z,W

  X = 2.0
  Y = 1.0
  Z = 2.0
  W = 3.0

  X = X + Y - Z + W ! CHECK: (((X+Y)-Z)+W)
  X = X + Y * Z ! CHECK: (X+(Y*Z))
  X = X * Y + Z ! CHECK: ((X*Y)+Z)
  X = (X + Y) * Z ! CHECK: ((X+Y)*Z)
  X = X * Y ** Z ! CHECK: (X*(Y**Z))
  X = X + Y ** Z / W ! CHECK: (X+((Y**Z)/W))
  X = X + Y ** (Z / W) ! CHECK: (X+(Y**(Z/W)))

  X = (X + Y) * Z - W ! CHECK: (((X+Y)*Z)-W)
  X = X + Y * -Z ! CHECK: (X+(Y*(-Z)))

  X = X + Y .EQ. Z ! CHECK: ((X+Y)==Z)
  X = X / Y .LT. Z ! CHECK: ((X/Y)<Z)
  X = X - Y .GT. Z ** W ! CHECK: ((X-Y)>(Z**W))

  X = X
  X = (X)
  X = (3 ! expected-error@+1 {{expected ')'}}
  X = ! expected-error@+1 {{expected an expression after '='}}
ENDPROGRAM expressions
