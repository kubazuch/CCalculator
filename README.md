# CCalculator
Basic calculator written in C that is able to handle Big Integers in arbirtrary (2-16) base. It was written as a final project for the Programming 1 course (1st semester, 2021/22) at the MiNI WUT faculty.

## Usage
```
filename.exe <input file> [output file]
```
Default output file name is `result.txt`.

## Input and output
This calculator has rather strict input format (as set by the instructor). Empty lines are essential!

### Base conversions
```
[from_base] [to_base]

[number]


```
For example:
```
9 11

47154234100282725257171762785285346007


```

### Arithmetic operations
Permited operations: `+`, `*`, `/`, `%` (modulo), `^` (exponentiation)
```
[operation] [base]

[number 1]

[number 2]


```
For example:
```
+ 16

D2A3F955D3619A3F3E385DC93E8EC

F98FE7C3997C0BE32FACA47849489


```
Additionally, one extra line at the end of file is needed, giving three total empty lines at the end.


Output is in nearly the same format -- the result is placed in between the two ending empty lines, so for example input
```
^ 5

4102

403


```
will yield given result
```
^ 5

4102

403

103314003340222242201400220023342034313142013132442331203340423131221020030444243412223320224422110341242233124223130410100312102344401311333043143112310202111022244404240242121243331342431123234224330034022044442141000034432014324133422314341434034012342114131114343401432133132434123421421300000011404100001102340031331043412312401233243300003400121322142131442004314224444344224421041032240104004213

```
