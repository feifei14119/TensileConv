# TensileConv
auto generate and tune convolution 1x1 forward kernel code to achieve peak performance 

1.Support fusion opts: convolution + relu/Prelu + bias 
2.Support brute-force search and genetic search 
3.Support different hardware ISA: gfx900 , gfx803
4.Supply C++ library and interface 
5.Support offline and online tuning 
