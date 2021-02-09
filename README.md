# bcts_quote

## demo4quote

set enviroment LOCAL=true TO COMPILE LOCALLY AND WITH GPROFTOOLS

## gprof

1. link with "-pg" argument in CMakeLists.txt
2. run program as usual. report will be generated automatically, default name is gmon.out
3. make the output readable. ( command: gprof demo4quote gmon.out > report.txt )
4. visualize. ( command: python gprof2dot.py -n 0.99 -e 0.99 report.txt | dot -Tsvg -o ast.svg )