# 中间代码生成

本章为实验三，任务是在词法分析、语法分析和语义分析程序的基础上，将 C--源代码翻译为中间代码。理论上中间代码在编译器的内部表示可以选用树形结构（抽象语法树）或者线形结构（三地址代码）等形式，为了方便检查程序，我们要求将中间代码输出成线性结构，从而可以使用我们提供的虚拟机小程序（附录 B）来测试中间代码的运行结果。 

需要注意的是，由于在后面的实验中还会用到本次实验已经写好的代码，因此保持一个良好的代码风格、系统地设计代码结构和各模块之间的接口对于整个实验来讲相当重要。 

# 3.1 实验内容

# 3.1.1 实验要求

在本次实验中，我们对输入的 C-- 语言源代码文件做如下假设（注意：假设 2 和假设 3 可能因后面的不同选做要求而有所改变）： 

1）假设 1：不会出现注释、八进制或十六进制整型常数、浮点型常数或者变量。 

2）假设 2：不会出现类型为结构体或高维数组（高于 1 维的数组）的变量。 

3）假设 3：任何函数参数都只能为简单变量，也就是说，结构体和数组都不会作为参数传入函数中。 

4）假设 4：没有全局变量的使用，并且所有变量均不重名。 

5）假设 5：函数不会返回结构体或数组类型的值。 

6）假设 6：函数只会进行一次定义（没有函数声明）。 

7）假设 7：输入文件中不包含任何词法、语法或语义错误。 

程序需要将符合以上假设的 C--源代码翻译为中间代码，中间代码的形式及操作规范如表 3-1 所示，表中的操作大致可以分为如下几类。 


表 3-1 中间代码的形式及操作规范


<table><tr><td>语 法</td><td>描 述</td></tr><tr><td>LABEL x :</td><td>定义标号 x</td></tr><tr><td>FUNCTION f :</td><td>定义函数 f</td></tr><tr><td>x := y</td><td>赋值操作</td></tr><tr><td>x := y + z</td><td>加法操作</td></tr><tr><td>x := y - z</td><td>减法操作</td></tr><tr><td>x := y * z</td><td>乘法操作</td></tr><tr><td>x := y / z</td><td>除法操作</td></tr><tr><td>x := &amp;y</td><td>取 y 的地址赋给 x</td></tr><tr><td>x := *y</td><td>取以 y 值为地址的内存单元的内容赋给 x</td></tr><tr><td>*x := y</td><td>取 y 值赋给以 x 值为地址的内存单元</td></tr><tr><td>GOTO x</td><td>无条件跳转至标号 x</td></tr><tr><td>IF x [relop] y GOTO z</td><td>如果 x 与 y 满足 [relop] 关系则跳转至标号 z</td></tr><tr><td>RETURN x</td><td>退出当前函数并返回 x 值</td></tr><tr><td>DEC x [size]</td><td>内存空间申请,大小为 4 的倍数</td></tr><tr><td>ARG x</td><td>传实参 x</td></tr><tr><td>x := CALL f</td><td>调用函数,并将其返回值赋给 x</td></tr><tr><td>PARAM x</td><td>函数参数声明</td></tr><tr><td>READ x</td><td>从控制台读取 x 的值</td></tr><tr><td>WRITE x</td><td>向控制台打印 x 的值</td></tr></table>

1）标号语句 LABEL 用于指定跳转目标，注意 LABEL 与 x 之间、x 与冒号之间都被空格或制表符隔开。 

2）函数语句 FUNCTION 用于指定函数定义，注意 FUNCTION 与 f 之间、f 与冒号之间都被空格或制表符隔开。 

3）赋值语句可以对变量进行赋值操作（注意赋值号前后都应由空格或制表符隔开）。赋值号左边的 x 一定是一个变量或者临时变量，而赋值号右边的 y 既可以是变量或临时变量，也可以是立即数。如果是立即数，则需要在其前面添加 “#” 符号。例如，如果要将常数 5 赋给临时变量 t1，可以写成 t1 := #5。 

4）算术运算操作包括加、减、乘、除四种操作（注意运算符前后都应由空格或制表符隔开）。赋值号左边的 x 一定是一个变量或者临时变量，而赋值号右边的 y 和 z 既 

可以是变量或临时变量，也可以是立即数。如果是立即数，则需要在其前面添加“#”符号。例如，如果要将变量a与常数5相加并将运算结果赋给b，则可以写成 $b := a + \#5$ 。 

5）赋值号右边的变量可以添加“&”符号对其进行取地址操作。例如，b := &a + #8 代表将变量 a 的地址加上 8 然后赋给 b。 

6）当赋值语句右边的变量 y 添加了 “*” 符号时代表读取以 y 的值作为地址的那个内存单元的内容，而当赋值语句左边的变量 x 添加了 “*” 符号时则代表向以 x 的值作为地址的那个内存单元写入内容。 

7）跳转语句分为无条件跳转和有条件跳转两种。无条件跳转语句 GOTO x 会直接将控制转移到标号为 x 的那一行，而有条件跳转语句（注意语句中变量、关系操作符前后都应该被空格或制表符分开）则会先确定两个操作数 x 和 y 之间的关系（相等、不等、小于、大于、小于等于、大于等于共 6 种），如果该关系成立则进行跳转，否则不跳转而直接将控制转移到下一条语句。 

8）返回语句 RETURN 用于从函数体内部返回值并退出当前函数，RETURN 后面可以跟一个变量，也可以跟一个常数。 

9）变量声明语句DEC用于为一个函数体内的局部变量声明其所需要的空间，该空间的大小以字节为单位。这个语句是专门为数组变量和结构体变量这类需要开辟一段连续的内存空间的变量所准备的。例如，如果我们需要声明一个长度为10的int类型数组a，则可以写成DEC a 40。对于那些类型不是数组或结构体的变量，直接使用即可，不需要使用DEC语句对其进行声明。变量的命名规范与之前的实验相同。另外，在中间代码中不存在作用域的概念，因此不同的变量一定要避免重名。 

10）与函数调用有关的语句包括 CALL、PARAM 和 ARG 三种。其中 PARAM 语句在每个函数开头使用，对于函数中形参的数目和名称进行声明。例如，若一个函数 func 有三个形参 a、b、c，则该函数的函数体内前三条语句为：PARAM a、PARAM b 和 PARAM c。CALL 和 ARG 语句负责进行函数调用。在调用一个函数之前，我们先使用 ARG 语句传入所有实参，随后使用 CALL 语句调用该函数并存储返回值。仍以函数 func 为例，如果我们需要依次传入三个实参 x、y、z，并将返回值保存到临时变量 t1 中，则可分别表述为：ARG z、ARG y、ARG x 和 t1 := CALL func。注意 ARG 传入参数的顺序和 PARAM 声明参数的顺序正好相反。ARG 语句的参数可以是变量、以 # 开头的常数或以 & 开头的某个变量的地址。 

11）输入输出语句 READ 和 WRITE 用于和控制台进行交互。READ 语句可以从控制台读入一个整型变量，而 WRITE 语句可将一个整型变量的值写到控制台上。 

除以上说明外，注意关键字及变量名都是大小写敏感的，也就是说“abc”和“AbC”会被作为两个不同的变量对待，上述所有关键字（例如CALL、IF、DEC等）都必须大写，否则虚拟机小程序会将其看作一个变量名。 

在实验三中，你可能需要在实验二的程序中做如下更改：在符号表中预先添加read和write这两个预定义的函数。其中read函数没有任何参数，返回值为int型（即读入的整数值），write函数包含一个int类型的参数（即要输出的整数值），返回值也为int型（固定返回0）。添加这两个函数的目的是让C--源程序拥有可以与控制台进行交互的接口。在中间代码翻译的过程中，read函数可直接对应READ操作，write函数可直接对应WRITE操作。 

除此之外，你的程序可以选择完成以下部分或全部的要求： 

1）要求3.1：修改前面对C--源代码的假设2和假设3，使源代码中： 

a）可以出现结构体类型的变量（但不会有结构体变量之间直接赋值）。 

b）结构体类型的变量可以作为函数的参数（但函数不会返回结构体类型的值）。 

2）要求3.2：修改前面对C--源代码的假设2和假设3，使源代码中： 

a）一维数组类型的变量可以作为函数参数（但函数不会返回一维数组类型的值）。 

b）可以出现高维数组类型的变量（但高维数组类型的变量不会作为函数的参数或返回类型的值）。 

此外，实验三还会考察你的程序输出的中间代码的执行效率，因此你需要考虑如何优化中间代码的生成。在程序可以生成正确的中间代码（“正确”是指该中间代码在虚拟机小程序上运行结果正确）的前提下，如果该中间代码在我们的测试用例上能比50%甚至80%的同学的中间代码效率都高，你将获得额外奖励。 

# 3.1.2 输入格式

程序的输入是一个包含 C--源代码的文本文件，程序需要能够接收一个输入文件名和一个输出文件名作为参数。例如，假设程序名为 cc、输入文件名为 test1、输出文件名为 out1.ir，程序和输入文件都位于当前目录下，那么在 Linux 命令行下运行 ./cc test1 out1.ir 即可将输出结果写入当前目录下名为 out1.ir 的文件中。 

# 3.1.3 输出格式

实验三要求程序将运行结果输出到文件。输出文件要求每行一条中间代码，每条中间代码的含义如前文所述。如果输入文件包含多个函数定义，则需要通过FUNCTION语句将这些函数隔开。FUNCTION语句和LABEL语句的格式类似，具体 

例子见后面的样例。 

对每个特定的输入，并不存在唯一正确的输出。我们将使用虚拟机小程序对中间代码的正确性进行测试。任何能被虚拟机小程序顺利执行并得到正确结果的输出都将被接受。此外，虚拟机小程序还会统计中间代码所执行过的各种操作的次数，以此来估计程序生成的中间代码的效率。 

# 3.1.4 测试环境

程序将在如下环境中被编译并运行： 

1) GNU Linux Release: Ubuntu 12.04, kernel version 3.2.0-29。 

2) GCC version 4.6.3。 

3) GNU Flex version 2.5.35。 

4) GNU Bison version 2.5。 

一般而言，只要避免使用过于冷门的特性，使用其他版本的 Linux 或者 GCC 等，也基本上不会出现兼容性方面的问题。注意，实验三的检查过程中不会去安装或尝试引用各类方便编程的函数库（如 glib 等），因此请不要在程序中使用它们。 

# 3.1.5 提交要求

实验三要求提交如下内容（同实验一）： 

1）Flex、Bison 以及 C 语言的可被正确编译运行的源代码程序。 

2）一份 PDF 格式的实验报告，内容包括： 

a）程序实现了哪些功能？简要说明如何实现这些功能。清晰的说明有助于助教对你的程序所实现的功能进行合理的测试。 

b）程序应该如何被编译？可以使用脚本、makefile或逐条输入命令进行编译，请详细说明应该如何编译你的程序。无法顺利编译将导致助教无法对你的程序所实现的功能进行任何测试，从而丢失相应的分数。 

c）实验报告的长度不得超过三页。所以实验报告中需要重点描述的是程序中的亮点，是你认为最个性化、最具独创性的内容，而相对简单的、任何人都可以做的内容则可不提或简单地提一下，尤其要避免大段地在报告里贴代码。实验报告中所出现的最小字号不得小于五号字（或英文11号字）。 

# 3.1.6 样例（必做内容）

实验三的样例包括必做内容样例与选做要求样例两部分，分别对应于实验要求中 的必做内容和选做要求。请仔细阅读样例，以加深对实验要求以及输出格式要求的理解。本节将列举必做内容样例。 

例 3.1: 

输入： 

```c
int main()
{
    int n;
    n = read();
    if (n > 0) write(1);
    else if (n < 0) write(-1);
    else write(0);
    return 0;
} 
```

输出： 

这段程序读入一个整数 n，然后计算并输出符号函数 $\mathrm{sgn}(x)$ 。它所对应的中间代码可以是这样的： 

```txt
1 FUNCTION main :
2 READ t1
3 v1 := t1
4 t2 := #0
5 IF v1 > t2 GOTO label1
6 GOTO label2
7 LABEL label1 :
8 t3 := #1
9 WRITE t3
10 GOTO label3
11 LABEL label2 :
12 t4 := #0
13 IF v1 < t4 GOTO label4
14 GOTO label5
15 LABEL label4 :
16 t5 := #1
17 t6 := #0 - t5
18 WRITE t6
19 GOTO label6
20 LABEL label5 :
21 t7 := #0
22 WRITE t7
23 LABEL label6 :
24 LABEL label3 :
25 t8 := #0
26 RETURN t8 
```

需要注意的是，虽然样例输出中使用的变量遵循着字母 t 后跟一个数字（如 t1、v1 等）的方式，标号也遵循着 label 后跟一个数字的方式，但这并不是强制要求的。也就 是说，程序输出完全可以使用其他符合变量名定义的方式而不会影响虚拟机小程序的运行。 

可以发现，这段中间代码中存在很多可以优化的地方。首先，0这个常数我们将其赋给了t2、t4、t7、t8这四个临时变量，实际上赋值一次就可以了。其次，对于t6的赋值我们可以直接写成 $t6 := \# - 1$ 而不必多进行一次减法运算。另外，程序中的标号也有些冗余。如果程序足够“聪明”，可能会将上述中间代码优化成这样： 

```txt
1 FUNCTION main :
2 READ t1
3 v1 := t1
4 t2 := #0
5 IF v1 > t2 GOTO label1
6 IF v1 < t2 GOTO label2
7 WRITE t2
8 GOTO label3
9 LABEL label1 :
10 t3 := #1
11 WRITE t3
12 GOTO label3
13 LABEL label2 :
14 t6 := #-1
15 WRITE t6
16 LABEL label3 :
17 RETURN t2 
```

# 例 3.2:

输入： 

```c
int fact(int n)
{
    if (n == 1)
    return n;
    else
    return (n * fact(n - 1));
}
int main()
{
    int m, result;
    m = read();
    if (m > 1)
    result = fact(m);
    else
    result = 1;
    write(result);
    return 0;
} 
```

输出： 

这是一个读入 m 并输出 m 的阶乘的小程序，其对应的中间代码可以是： 

```txt
1 FUNCTION fact :
2 PARAM v1
3 IF v1 == #1 GOTO label1
4 GOTO label2
5 LABEL label1 :
6 RETURN v1
7 LABEL label2 :
8 t1 := v1 - #1
9 ARG t1
10 t2 := CALL fact
11 t3 := v1 * t2
12 RETURN t3
13
14 FUNCTION main :
15 READ t4
16 v2 := t4
17 IF v2 > #1 GOTO label3
18 GOTO label4
19 LABEL label3 :
20 ARG v2
21 t5 := CALL fact
22 v3 := t5
23 GOTO label5
24 LABEL label4 :
25 v3 := #1
26 LABEL label5 :
27 WRITE v3
28 RETURN #0 
```

这个样例主要展示如何处理包含多个函数以及函数调用的输入文件。 

# 3.1.7 样例（选做要求）

本节将列举选做要求样例。 

例 3.3: 

输入： 

```c
struct Operands
{
    int o1;
    int o2;
};
int add(struct Operands temp)
{
    return (temp.o1 + temp.o2);
}
int main()
{
    int n; 
```

```solidity
15 struct Operands op;
16 op.01 = 1;
17 op.02 = 2;
18 n = add(op);
19 write(n);
20 return 0;
21 } 
```

输出： 

样例输入中出现了结构体类型的变量以及这样的变量作为函数参数的用法。如果你的程序需要完成要求 3.1，样例输入对应的中间代码可以是： 

```txt
1 FUNCTION add :
2 PARAM v1
3 t2 := *v1
4 t7 := v1 + #4
5 t3 := *t7
6 t1 := t2 + t3
7 RETURN t1
8 FUNCTION main :
9 DEC v3 8
10 t9 := &v3
11 *t9 := #1
12 t12 := &v3 + #4
13 *t12 := #2
14 ARG &v3
15 t14 := CALL add
16 v2 := t14
17 WRITE v2
18 RETURN #0 
```

如果程序不需要完成要求 3.1，将不能翻译该样例输入，程序可以给出如下提示信息： 

```txt
Cannot translate: Code contains variables or parameters of structure type. 
```

例 3.4: 

输入： 

```c
int add(int temp[2])
{
    return (temp[0] + temp[1]);
}

int main()
{
    int op[2];
    int r[1][2];
    int i = 0, j = 0;
    while (i < 2) 
```

```txt
12    {
13    while (j < 2)
14    {
15    op[j] = i + j;
16    j = j + 1;
17    }
18    r[0][i] = add(op);
19    write(r[0][i]);
20    i = i + 1;
21    j = 0;
22    }
23    return 0;
24 } 
```

输出： 

样例输入中出现了高维数组类型的变量以及一维数组类型的变量作为函数参数的用法。如果程序需要完成要求 3.2，样例输入对应的中间代码可以是： 

```txt
1 FUNCTION add :
2 PARAM v1
3 t2 := *v1
4 t11 := v1 + #4
5 t3 := *t11
6 t1 := t2 + t3
7 RETURN t1
8 FUNCTION main :
9 DEC v2 8
10 DEC v3 8
11 v4 := #0
12 v5 := #0
13 LABEL label1 :
14 IF v4 < #2 GOTO label2
15 GOTO label3
16 LABEL label2 :
17 LABEL label4 :
18 IF v5 < #2 GOTO label5
19 GOTO label6
20 LABEL label5 :
21 t18 := v5 * #4
22 t19 := &v2 + t18
23 t20 := v4 + v5
24 *t19 := t20
25 v5 := v5 + #1
26 GOTO label4
27 LABEL label6 :
28 t31 := v4 * #4
29 t32 := &v3 + t31
30 ARG &v2
31 t33 := CALL add
32 *t32 := t33
33 t41 := v4 * #4
34 t42 := &v3 + t41 
```

```asm
35 t35 := *t42
36 WRITE t35
37 v4 := v4 + #1
38 v5 := #0
39 GOTO label1
40 LABEL label3 :
41 RETURN #0 
```

如果程序不需要完成要求 3.2，将不能翻译该样例输入，程序可以给出如下提示信息： 

Cannot translate: Code contains variables of multi-dimensional array type or parameters of array type. 

# 3.2 实验指导

编译器里最核心的数据结构之一就是中间代码（Intermediate Representation 或 IR）。中间代码应包含哪些信息，这些信息又应有怎样的内部表示？这些问题会极大地影响编译器代码的复杂程度、编译器的运行效率以及编译生成的目标代码的运行效率。 

广义地说，编译器根据输入程序所构造出来的绝大多数数据结构都被称为中间代码（或可更精确地译为“中间表示”）。例如，我们之前所构造的词法流、语法树、带属性的语法树等，都可视为中间代码。使用中间代码的主要原因是为了方便编写编译器程序的各种操作。如果我们在需要有关输入程序的任何信息时都只能去重新读入并处理输入程序源代码的话，编译器的编写将会变得非常麻烦，同时也会大大降低其运行效率。 

狭义地说，中间代码是编译器从源语言到目标语言之间采用的一种过渡性质的代码形式（这时它常被称作 Intermediate Code）。你可能会有疑问：为什么编译器不能把输入程序直接翻译成目标代码，而是要额外引入中间代码呢？实际上，引入中间代码有两个主要的好处。一方面，中间代码将编译器自然地分为前端和后端两个部分。当我们需要改变编译器的源语言或目标语言时，如果采用了中间代码，我们只需要替换原编译器的前端或后端，而不需要重写整个编译器。另一方面，即使源语言和目标语言是固定的，采用中间代码也有利于编译器的模块化。人们将编译器设计中那些复杂但相关性不大的任务分别放在前端和后端的各个模块中，这既简化了模块内部的处理，又使我们能单独对每个模块进行调试与修改而不影响其他模块。下文中，如果不特别说明，“中间代码”都是指狭义的中间代码。 

# 3.2.1 中间代码的分类

中间代码的设计可以说更多的是一门艺术而不是技术。不同编译器所使用的中间代码可能是千差万别的，即使是同一编译器内部也可以使用多种不同的中间代码，有的中间代码与源语言更接近，有的中间代码与目标语言更接近。编译器需要在不同的中间代码之间进行转换，有时为了处理的方便，甚至会在将中间代码1转换为中间代码2之后，对中间代码2进行优化然后又转换回中间代码1。这些不同的中间代码虽然对应了同一输入程序，但它们却体现了输入程序不同层次上的细节信息。举个实际的例子：GCC内部首先会将输入程序转换成一棵抽象语法树，然后将该树转换为另一种被称为GIMPLE的树形结构。在GIMPLE之上它建立静态单赋值式的中间代码之后，又会将其转换为一种非常底层的RTL（Register Transfer Language）代码，最后才把RTL转换为汇编代码。 

我们可以从不同的角度对现存的这些花样繁多的中间代码进行分类。从中间代码所体现出的细节上，我们可以将中间代码分为如下三类： 

1）高层次中间代码（High-level IR 或 HIR）：这种中间代码体现了较高层次的细节信息，因此往往和高级语言类似，保留了不少包括数组、循环在内的源语言的特征。高层次中间代码常在编译器的前端部分使用，并在之后被转换为更低层次的中间代码。高层次中间代码常被用于进行相关性分析（Dependence Analysis）和解释执行。我们所熟悉的 Java bytecode、Python .pyc bytecode 以及目前使用得非常广泛的 LLVM IR 都属于高层次 IR。 

2）中层次中间代码（Medium-level IR 或 MIR）：这个层次的中间代码在形式上介于源语言和目标语言之间，它既体现了许多高级语言的一般特性，又可以被方便地转换为低级语言的代码。正是由于 MIR 的这个特性，它是三种 IR 中最难设计的一种。在这个层次上，变量和临时变量可能已经有了区分，控制流也可能已经被简化为无条件跳转、有条件跳转、函数调用和函数返回四种操作。另外，对中层次中间代码可以进行绝大部分的优化处理，例如公共子表达式消除（Common-subexpression Elimination）、代码移动（Code Motion）、代数运算简化（Algebraic Simplification）等。 

3）低层次中间代码（Low-level IR 或 LIR）：低层次中间代码与目标语言非常接近，它在变量的基础上可能会加入寄存器的细节信息。事实上，LIR 中的大部分代码和目标语言中的指令往往存在着一一对应的关系，即使没有对应，二者之间的转换也属于一次指令选择就能完成的任务。前面提到的 RTL 就属于一种非常典型的低层次 IR。 

图 3-1 给出了一个完成相同功能的三种 IR 的例子（从左到右依次为 HIR、MIR 和 LIR）。 

<table><tr><td>t1 = a[i][j+2]</td><td>t1 = j + 2</td><td>r1 = [fp - 4]</td></tr><tr><td></td><td>t2 = i * 20</td><td>r2 = r1 + 2</td></tr><tr><td></td><td>t3 = t1 + t2</td><td>r3 = [fp - 8]</td></tr><tr><td></td><td>t4 = 4 * t3</td><td>r4 = r3 * 20</td></tr><tr><td></td><td>t5 = addr a</td><td>r5 = r4 + r2</td></tr><tr><td></td><td>t6 = t5 + t4</td><td>r6 = 4 * r5</td></tr><tr><td></td><td>t7 = *t6</td><td>r7 = fp - 216</td></tr><tr><td></td><td></td><td>f1 = [r7 + r6]</td></tr></table>


图 3-1 三种不同层次的中间代码示例


从表示方式来看，我们又可以将中间代码分成如下三类： 

1）图形中间代码（Graphical IR）：这种类型的中间代码将输入程序的信息嵌入到一张图中，以结点和边等元素来组织代码信息。由于要表示和处理一般的图代价会很大，人们经常会使用特殊的图，例如树或有向无环图（DAG）。一个典型的树形中间代码的例子就是抽象语法树（Abstract Syntax Tree 或 AST）。抽象语法树中省去了语法树里不必要的结点，将输入程序的语法信息以一种更加简洁的形式呈现出来。其他树形中间代码的例子有 GCC 中所使用的 GIMPLE。这类中间代码将各种操作都组织在一棵树中，在后面实验四的指令选择部分我们会看到这种表示方式可以简化其中的某些处理。 

2）线形中间代码（Linear IR）：线形结构的代码我们见得非常多，例如我们经常使用的 C 语言、Java 语言和汇编语言中语句和语句之间就是线性关系。你可以将这种中间代码看成是某种抽象计算机的一个简单的指令集。这种结构最大的优点是表示简单、处理高效，而缺点就是代码和代码之间的先后关系有时会模糊整段程序的逻辑，让某些优化操作变得很复杂。 

3）混合型中间代码（Hybrid IR）：顾名思义，混合型中间代码主要混合了图形和线形两种中间代码，期望结合这两种代码的优点并避免二者的缺点。例如，我们可以将中间代码组织成许多基本块，块内部采用线形表示，块与块之间采用图表示，这样既可以简化块内部的数据流分析，又可以简化块与块之间的控制流分析。 

在实验三中，你需要按照格式输出中间代码。虽然实验要求中规定的中间代码格式类似于线形的中层次中间代码，但这只是输出格式，而你的程序内部可以采用任何形式的中间代码，而这些中间代码中又体现了多少细节信息，则完全取决于你自己的设计。 

# 3.2.2 中间代码的表示（线形）

在实验三中，你可能会一边对语法树进行处理一边把要输出的代码内容打印出来。这种做法其实并不好，因为当代码内容被打印出来的那一刻起，我们就已经失去了对这些代码进行调整和优化的机会。更加合理的做法是将所生成的中间代码先保存到内存中，等全部翻译完毕，优化也都做完后再使用一个专门的打印函数把在内存中的中间代码打印出来。既然生成好的中间代码会被放到内存中，那么如何保存这些代码以及为其设计怎样的数据结构就是值得考虑的问题了。我们下面对一种典型的线形 IR 的实现细节进行介绍，关于树形 IR 的实现细节我们将放到下一节介绍。 

相对而言，线形 IR 是实现起来最简单，而且打印起来最方便的中间代码形式。由于代码本身是线形的，我们可以使用几种最基本的线形数据结构来表示它们，如图 3-2 所示。 

<table><tr><td>Target</td><td>Op</td><td>Arg1</td><td>Arg2</td></tr><tr><td><eq>{\mathrm{t}}_{1}</eq></td><td>←</td><td>2</td><td></td></tr><tr><td><eq>{\mathrm{t}}_{2}</eq></td><td>←</td><td>b</td><td></td></tr><tr><td><eq>{\mathrm{t}}_{3}</eq></td><td>✘</td><td><eq>{\mathrm{t}}_{1}</eq></td><td><eq>{\mathrm{t}}_{2}</eq></td></tr><tr><td><eq>{\mathrm{t}}_{4}</eq></td><td>←</td><td>a</td><td></td></tr><tr><td><eq>{\mathrm{t}}_{5}</eq></td><td>-</td><td><eq>{\mathrm{t}}_{4}</eq></td><td><eq>{\mathrm{t}}_{3}</eq></td></tr></table>


a) 静态数组


![](images/ac3fc11651e772d4e3798f8a3ff471e3002be3d62d6c1524baf06a5564f96713.jpg)



b) 指针型数组


![](images/6960afc34ce6173171312cd065db515f82127bf0ebd6b815a47be3548dbcf671.jpg)



c) 链表



图 3-2 表示线性 IR 的三种基本数据结构 $^{①}$


图 3-2a 为一个大的静态数组，数组中的每个元素（图中的一行）就是一条中间代码。使用静态数组的好处是写起来编程方便，缺点是灵活性不足。中间代码的最大行数受限，而且代码的插入、删除以及调换位置的代价较大。图 3-2b 同样为一个大数组，但数组中的每个元素并不是一条中间代码，而是一个指向中间代码指针。虽然采用这种实现时代码行数也会受限，不过它和图 3-2a 的实现相比则大大减少了调换代码位置的开销。图 3-2c 是一个纯链表的实现，图中画出来的链表是单向的，但我们更建议使用双向循环链表。链表以增加实现的复杂性为代价换得了极大的灵活性，可以进行高效的插入、删除以及调换位置操作，并且几乎不存在代码最大行数的限制。 

假设单条中间代码的数据结构定义为： 

```c
1 typedef struct Operand_* Operand;
2 struct Operand_ { . 
```

```txt
enum { VARIABLE, CONSTANT, ADDRESS, ... } kind;
union {
    int var_no;
    int value;
    ...
} u;
};
struct InterCode
{
    enum { ASSIGN, ADD, SUB, MUL, ... } kind;
    union {
    struct { Operand right, left; } assign;
    struct { Operand result, op1, op2; } binop;
    ...
} u;
} 
```

那么，图 3-2a 的实现可以写成： 

```txt
InterCode codes[MAX_LINE]; 
```

图 3-2b 的实现可以写成: 

```txt
InterCode* codes[MAX_LINE]; 
```

图 3-2c 的（双向链表）实现可以写成： 

```c
struct InterCodes { InterCode code; struct InterCodes *prev, *next; }; 
```

要想打印出线形 IR 非常简单，只需从第一行代码开始逐行访问，根据每行代码 kind 域的不同值按照不同的格式打印即可。对于数组，逐行访问其实就意味着一个 for 循环；对于链表，逐行访问则意味着以一个 while 循环顺着链表的 next 域进行迭代。 

# 3.2.3 中间代码的表示（树形）

树形 IR 看上去可能让人感到复杂，但仔细想想就会发现其实它与线形 IR 一样直观。树形结构天然具有层次的概念，在靠近树根的高层部分的中间代码其抽象层次较高，而靠近树叶的低层部分的中间代码则更加具体。例如，中间代码 t1 := v2 + #3 可用树形结构表示为如图 3-3 所示。 

![](images/75ec1a704fcc51b9bc8129c576665ca3ac5bb60f34b7594b549da9a3a2e89dfb.jpg)



图 3-3 中间代码 t1 := v2 + #3 的树形结构表示


我们也可以从另一个角度来理解树形 IR。在实验一中，我们曾为输入程序构造过语法树，这棵语法树所对应的程序设计语言是编译器的源语言；而在这里，树形结构同样可以看作是一棵语法树，它所对应的程序设计语言则是我们的中间代码。源语言的语法相当复杂，但实验三要求我们输出的中间代码的语法规则却是简单的，因此树形结构的中间代码的设计与实现也会比语法树更简单。 

之前我们已经做过有关语法树的实验，你对树形结构该如何实现应当非常熟悉。正如前面所述，树形 IR 可以看作是一种基于中间代码的语法树（或抽象语法树），因此其数据结构以及实现细节与语法树非常类似。有了写语法树的经验，写树形 IR 只需在原有基础上稍加修改即可，不会带来太多困难。 

树形 IR 的打印要比线形 IR 复杂一些，该任务类似于给定一棵输入程序的语法树需要将该输入程序打印出来。你需要对树形 IR 进行（深度优先）遍历，根据当前结点的类型递归地对其各个子结点进行打印。从另外一个角度看，从之前实验中构造的语法树到实验三要求输出的中间代码之间总要经历一个由树形到线形的转换，使用线形 IR 其实就是将这步转换提前到构造 IR 时，而使用树形 IR 则是将这步转换推后到输出时才进行。 

# 3.2.4 初探运行时环境

在一个程序员眼里，程序设计语言中可以有很多机制，包括类型（基本类型、数组和结构体）、类和对象、异常处理、动态类型、作用域、函数调用、名空间等，而且每个程序似乎都有使用不完的内存空间。但很显然，程序运行所基于的底层硬件不能支持这么多机制。一般来说，硬件只对32或64位整数、IEEE浮点数、简单的算术运算、数值拷贝以及简单地跳转提供直接的支持，并且其存储器的大小也是有限的、结构也是分层的。程序设计语言中的其他机制则需要编译器、汇编/链接器、操作系统等共同努力，从而让程序员们产生一种幻觉，认为他们眼中所看到的所有机制都是被底层硬件直接支持的。 

使用程序设计语言所书写出来的变量、类、函数等都是些抽象层次比较高的概念，为了能使用硬件直接支持的底层操作来表示这些高抽象层次的概念，仅靠编译时字面上的代码翻译是远远不够的，我们需要能够生成额外的目标代码，使程序在运行时可以维护一系列的结构以支撑起程序设计语言中的各种高级特性。这些程序员一般不可见、但又确实存在于运行时刻的结构就被称为运行时环境（Runtime Environment）。运行时环境与源语言、目标语言和目标机器都紧密相关，由于其中包含的很多细节并不是一两句话就能够说明清楚，因此对于运行时环境这部分内容我们会分拆到实验三 和实验四中逐步细化。在实验三中，我们以介绍原理为主，附带介绍一些简单结构（例如数组和结构体）的实现方式。在后面的实验四中，我们会更加细致地考查 MIPS 体系结构下的寄存器规约以及调用栈的布置。 

高级语言中的 char、short 和 int 等类型一般会直接对应到底层机器上的一个、两个或四个字节，而 float 和 double 类型则会对应到底层机器上的四个和八个字节，这些类型都可以由硬件直接提供支持。底层硬件中没有指针类型，但指针可以用四个字节（32 位机器）或者八个字节（64 位机器）整数表示，其内容即为指针所指向的内存地址。 

以上是一些比较基本的类型，下面我们来考察一维数组的表示。最熟悉的表示数组的方式是 C 风格的（如图 3-4 所示），即数组中的元素一个挨着一个并占用一段连续的内存空间 $^{①}$ 。 

![](images/51b1e44abf094fd82b512738054f24610b838cd201758a4bbba97d811abb5996.jpg)



图 3-4 C 语言中一维数组的内存表示方式


当然这不是唯一的表示方法。Java 在编译 bytecode 时就会采取另外一种布局，将数组长度放在起始位置（Pascal 中的 string 类型数据也是这样保存的），如图 3-5 所示。 

<table><tr><td>n</td><td>Arr[0]</td><td>Arr[1]</td><td>Arr[2]</td><td>...</td><td>Arr[n-1]</td></tr></table>


图 3-5 Java 语言中一维数组的内存表示方式


另外还有一种表示方式为 D 语言所采用：数组变量本身仅由两个指针组成，一个指向数组的开头，另一个指向数组的末尾之后，数组的所有信息存在于另外一段内存之中，如图 3-6 所示。 

![](images/6292d3e37863e9aed51eb1f8d3f9fe087e014deaf75b1e28180210fa33a86b56.jpg)



图 3-6 D 语言中一维数组的内存表示方式


可以看出，无论是哪一种表示方式，数组元素在内存中总是连续存储的，这当然是为了使数组的访问能够更快（只需计算基地址 + 偏移量，然后取值即可）。多维数组可以看作一维数组的数组，C 风格的表示方法仍然是使用一段连续的内存空间，如图 3-7 所示。 

<table><tr><td>a[0][0]</td><td>a[0][1]</td><td>a[1][0]</td><td>a[1][1]</td><td>a[2][0]</td><td>a[2][1]</td></tr></table>


图 3-7 C 语言中多维数组的内存表示方式


而 Java 中每个一维数组是一个独立的对象，因此多维数组中的各维一般不会聚在一起，如图 3-8 所示。 

![](images/c104b3cf9835c205c3642575d707f642afb74fd703e197adaef94eaf72bc5258.jpg)



图 3-8 Java 语言中多维数组的内存表示方式


结构体的表示与数组类似，最常见的办法是将各个域按定义的顺序连续地存放在一起。比如 struct { int a; float b[2]; } 在内存中的表示如图 3-9 所示。 

![](images/db07b5840ef13304385e254f9d0f94a956a5fefd3f408771b3bc975b9d3309cc.jpg)



图 3-9 结构体的内存表示示例


我们的实验三中只包含 int 和 float 两种类型，而这两种类型的宽度都是 4 字节，这省去了我们许多的麻烦。如果 C-- 语言允许其他宽度的类型存在又会如何呢？例如，struct { int a; char b; double c; } 这个结构体在内存中会排成如图 3-10 所示的表示方式吗？ 

![](images/499c0209ecfd4f84ac0a14128bc3c6f806c15e0548b42ad71beed4a07d23eccd.jpg)



图 3-10 C--语言中结构体的内存表示的错误示例


答案是不会。如果没有特别指定，GCC 总会将结构体中的域对齐到字边界。因此在 char b 和 double c 之间会有 3 字节的空间被浪费掉，如图 3-11 所示。 

![](images/8aa2480a8f96ba4a5ff1a5c5eadad97b60c9318606ec4fa763bf6b7e0ae11487.jpg)



图 3-11 C--语言中结构体的内存表示的正确示例


x86 平台允许变量在存储时不对齐，但 MIPS 平台则要求对齐，这是我们需要注意的。 

最后我们简单讨论一下函数的表示。众所周知，在调用函数时我们需要进行一系列的配套工作，包括找到函数的入口地址、参数传递、为局部变量申请空间、保存寄存器现场、返回值传递和控制流跳转等。在这一过程中，我们需要用到的各种信 息都必须有地方能够保存下来，而保存这些信息的结构就称为活动记录（Activation Record）。因为活动记录经常被保存在栈上，故它往往也被称为栈帧（Stack Frame）。活动记录的建立是维护运行时刻环境的重点，编译器一般也会为它生成大段的额外代码，这意味着函数调用的开销一般会很大。所以对于功能简单的函数来说，内联展开往往是一种有效的优化方法。在实验三中我们并不需要关心活动记录是如何建立的，只需要压入相应的参数然后调用 CALL 语句即可。但要注意的是，若数组或结构体作为参数传递，需谨记数组和结构体都要采用引用调用（Call by Reference）的参数传递方式，而非普通变量的值调用（Call by Value）。 

# 3.2.5 翻译模式（基本表达式）

实验三的任务比较简单，你只需根据语法树产生出中间代码，然后将中间代码按照输出格式打印出来即可。中间代码如何表示以及如何打印我们都已经讨论过了，现在需要解决的问题是：如何将语法树变成中间代码？ 

最简单也是最常用的方式仍是遍历语法树中的每一个结点，当发现语法树中有特定的结构出现时，就产生出相应的中间代码。和语义分析一样，中间代码的生成需要借助于实验二中我们已经提到的工具：语法制导翻译（SDT）。具体到代码上，我们可以为每个主要的语法单元“X”都设计相应的翻译函数“translate_X”，对语法树的遍历过程也就是这些函数之间互相调用的过程。每种特定的语法结构都对应了固定模式的翻译“模板”，下面我们针对一些典型的语法树结构的翻译“模板”进行说明。这些内容你也可以在课本上找到，课本上介绍的翻译模式与下面我们介绍的可能略有不同，但核心思想是一致的 $^{①}$ 。 

我们先从语言最基本的结构表达式开始。 

表 3-2 列出了与表达式相关的一些结构的翻译模式。假设我们有函数 translate_Exp()，它接受三个参数：语法树的结点 Exp、符号表 sym_table 以及一个变量名 place，并返回一段语法树当前结点及其子孙结点对应的中间代码（或是一个指向存储中间代码内存区域的指针）。 

根据语法单元 Exp 所采用的产生式的不同，我们将生成不同的中间代码： 

1）如果 Exp 产生了一个整数 INT，那么我们只需要为传入的 place 变量赋值成前面加上一个 “#” 的相应数值即可。 


表 3-2 基本表达式的翻译模式


<table><tr><td colspan="2">translate_Exp(Exp, sym_table, place) = case Exp of</td></tr><tr><td>INT</td><td>value = get_value(INT)return [place := #value]<eq>^1</eq></td></tr><tr><td>ID</td><td>variable = lookup(sym_table, ID)return [place := variable.name]</td></tr><tr><td>Exp<eq>_1</eq> ASSIGNOP Exp<eq>_2</eq><eq>(Exp_{1} \rightarrow ID)</eq></td><td>variable = lookup(sym_table, Exp<eq>_1</eq>.ID)t1 = new_temp()code1 = translate_Exp(Exp<eq>_2</eq>, sym_table, t1)code2 = [variable.name := t1] + <eq>^3</eq>[place := variable.name]return code1 + code2</td></tr><tr><td>Exp<eq>_1</eq> PLUS Exp<eq>_2</eq></td><td>t1 = new_temp()t2 = new_temp()code1 = translate_Exp(Exp<eq>_1</eq>, sym_table, t1)code2 = translate_Exp(Exp<eq>_2</eq>, sym_table, t2)code3 = [place := t1 + t2]return code1 + code2 + code3</td></tr><tr><td>MINUS Exp<eq>_1</eq></td><td>t1 = new_temp()code1 = translate_Exp(Exp<eq>_1</eq>, sym_table, t1)code2 = [place := #0 - t1]return code1 + code2</td></tr><tr><td>Exp<eq>_1</eq> RELOP Exp<eq>_2</eq></td><td rowspan="4">label1 = new_label()label2 = new_label()code0 = [place := #0]code1 = translate_Cond(Exp, label1, label2, sym_table)code2 = [LABEL label1] + [place := #1]return code0 + code1 + code2 + [LABEL label2]</td></tr><tr><td>NOT Exp<eq>_1</eq></td></tr><tr><td>Exp<eq>_1</eq> AND Exp<eq>_2</eq></td></tr><tr><td>Exp<eq>_1</eq> OR Exp<eq>_2</eq></td></tr></table>


① 用方括号括起来的内容表示新建一条具体的中间代码。 



② 这里 Exp 的下标只是用来区分产生式 Exp → Exp ASSIGNOP Exp 中多次重复出现的 Exp。 



③ 这里的加号相当于连接运算，表示将两段代码连接成一段。 


2）如果 Exp 产生了一个标识符 ID，那么我们只需要为传入的 place 变量赋值成 ID 对应的变量名（或该变量对应的中间代码中的名字）即可。 

3）如果 Exp 产生了赋值表达式 $Exp_{1}$ ASSIGNOP $Exp_{2}$ ，由于之前提到过作为左值的 $Exp_{1}$ 只能是三种情况之一（单个变量访问、数组元素访问或结构体特定域的访问），而对于数组和结构体的翻译模式我们将在后面讨论，故这里仅列出当 $Exp_{1} \rightarrow ID$ 时应该如何进行翻译。我们需要通过查表找到 ID 对应的变量，然后对 $Exp_{2}$ 进行翻译（运算结果储存在临时变量 t1 中），再将 t1 中的值赋予 ID 所对应的变量并将结果再存回 place，最后把刚翻译好的这两段代码合并随后返回即可。 

4）如果 Exp 产生了算术运算表达式 $Exp_{1}$ PLUS $Exp_{2}$ ，则先对 $Exp_{1}$ 进行翻译（运算结果储存在临时变量 t1 中），再对 $Exp_{2}$ 进行翻译（运算结果储存在临时变量 t2 中），最后生成一句中间代码 place := t1 + t2，并将刚翻译好的这三段代码合并后返回即可。 

使用类似的翻译模式我们也可以对减法、乘法和除法表达式进行翻译。 

5）如果 Exp 产生了取负表达式 MINUS Exp1，则先对 Exp1 进行翻译（运算结果储存在临时变量 t1 中），再生成一句中间代码 place := #0 - t1 从而实现对 t1 取负，最后将翻译好的这两段代码合并后返回。使用类似的翻译模式我们也可以对括号表达式进行翻译。 

6）如果 Exp 产生了条件表达式（包括与、或、非运算以及比较运算的表达式），我们则会调用 translate_Cond 函数进行（短路）翻译。如果条件表达式为真，那么为 place 赋值 1；否则，为其赋值 0。由于条件表达式的翻译可能与跳转语句有关，表中并没有明确说明 translate_Cond 该如何实现，这一点我们在后面介绍。 

# 3.2.6 翻译模式（语句）

C 的语句包括表达式语句、复合语句、返回语句、跳转语句和循环语句，它们的翻译模式如表 3-3 所示。 


表 3-3 语句的翻译模式


<table><tr><td colspan="2">translate_Stmt(Stmt, sym_table) = case Stmt of</td></tr><tr><td>Exp SEMI</td><td>return translate_Exp(Exp, sym_table, NULL)</td></tr><tr><td>CompSt</td><td>return translate_CompSt(CompSt, sym_table)</td></tr><tr><td>RETURN Exp SEMI</td><td>t1 = new_temp()code1 = translate_Exp(Exp, sym_table, t1)code2 = [RETURN t1]return code1 + code2</td></tr><tr><td>IF LP Exp RP Stmt1</td><td>label1 = new_label()label2 = new_label()code1 = translate_Cond(Exp, label1, label2, sym_table)code2 = translate_Stmt(Stmt1, sym_table)return code1 + [LABEL label1] + code2 + [LABEL label2]</td></tr><tr><td>IF LP Exp RP Stmt1ELSE Stmt2</td><td>label1 = new_label()label2 = new_label()label3 = new_label()code1 = translate_Cond(Exp, label1, label2, sym_table)code2 = translate_Stmt(Stmt1, sym_table)code3 = translate_Stmt(Stmt2, sym_table)return code1 + [LABEL label1] + code2 + [GOTO label3] + [LABEL label2] + code3 + [LABEL label3]</td></tr><tr><td>WHILE LP Exp RP Stmt1</td><td>label1 = new_label()label2 = new_label()label3 = new_label()code1 = translate_Cond(Exp, label2, label3, sym_table)code2 = translate_Stmt(Stmt1, sym_table)return [LABEL label1] + code1 + [LABEL label2] + code2 + [GOTO label1] + [LABEL label3]</td></tr></table>

你可能注意到，无论是 if 语句还是 while 语句，表 3-3 中列出的翻译模式都不包 含条件跳转。其实我们是在翻译条件表达式的同时生成这些条件跳转语句，translateCond函数负责对条件表达式进行翻译，其翻译模式如表3-4所示。 


表 3-4 条件表达式的翻译模式


<table><tr><td colspan="2">translate_Cond(Exp, label_true, label_false, sym_table) = case Exp of</td></tr><tr><td><eq>Exp_1</eq> RELOP <eq>Exp_2</eq></td><td>t1 = new_temp()t2 = new_temp()code1 = translate_Exp(<eq>Exp_1</eq>, sym_table, t1)code2 = translate_Exp(<eq>Exp_2</eq>, sym_table, t2)op = get_relop(RELOP);code3 = [IF t1 op t2 GOTO label_true]return code1 + code2 + code3 + [GOTO label_false]</td></tr><tr><td>NOT <eq>Exp_1</eq></td><td>return translate_Cond(<eq>Exp_1</eq>, label_false, label_true, sym_table)</td></tr><tr><td><eq>Exp_1</eq> AND <eq>Exp_2</eq></td><td>label1 = new_label()code1 = translate_Cond(<eq>Exp_1</eq>, label1, label_false, sym_table)code2 = translate_Cond(<eq>Exp_2</eq>, label_true, label_false, sym_table)return code1 + [LABEL label1] + code2</td></tr><tr><td><eq>Exp_1</eq> OR <eq>Exp_2</eq></td><td>label1 = new_label()code1 = translate_Cond(<eq>Exp_1</eq>, label_true, label1, sym_table)code2 = translate_Cond(<eq>Exp_2</eq>, label_true, label_false, sym_table)return code1 + [LABEL label1] + code2</td></tr><tr><td>(other cases)</td><td>t1 = new_temp()code1 = translate_Exp(Exp, sym_table, t1)code2 = [IF t1 != #0 GOTO label_true]return code1 + code2 + [GOTO label_false]</td></tr></table>

对于条件表达式的翻译，课本上已经花了较大的篇幅进行介绍，尤其是与回填有关的内容更是重点。不过，表3-4中没有与回填相关的任何内容。原因很简单：我们将跳转的两个目标label_true和label_false作为继承属性（函数参数）进行处理，在这种情况下每当我们在条件表达式内部需要跳转到外面时，跳转目标都已经从父结点那里通过参数得到了，直接填上即可。所谓回填，只用于将label_true和label_false作为综合属性处理的情况，注意这两种处理方式的区别。 

# 3.2.7 翻译模式（函数调用）

函数调用是由语法单元 Exp 推导而来的，因此，为了翻译函数调用表达式我们需要继续完善 translate_Exp，如表 3-5 所示。 

由于实验要求中规定了两个需要特殊对待的函数 read 和 write，故当我们从符号表中找到 ID 对应的函数名时不能直接生成函数调用代码，而是应该先判断函数名是否为 read 或 write。对于那些非 read 和 write 的带参数的函数而言，我们还需要调用 translate_Args 函数将计算实参的代码翻译出来，并构造这些参数所对应的临时变量列表 arg_list。translate_Args 的实现如表 3-6 所示。 


表 3-5 函数调用的翻译模式


<table><tr><td colspan="2">translate_Exp(Exp, sym_table, place) = case Exp of</td></tr><tr><td>ID LP RP</td><td>function = lookup(sym_table, ID)if (function.name == &quot;read&quot;) return [READ place]return [place := CALL function.name]</td></tr><tr><td>ID LPArgs RP</td><td>function = lookup(sym_table, ID)arg_list = NULLcode1 = translate_Args(Args, sym_table, arg_list)if (function.name == &quot;write&quot;) return code1 + [WRITE arg_list[1]]for i = 1 to length(arg_list) code2 = code2 + [ARG arg_list[i]]return code1 + code2 + [place := CALL function.name]</td></tr></table>


表 3-6 函数参数的翻译模式


<table><tr><td colspan="2">translate_Args(Args, sym_table, arg_list) = case Args of</td></tr><tr><td>Exp</td><td>t1 = new_temp()code1 = translate_Exp(Exp, sym_table, t1)arg_list = t1 + arg_listreturn code1</td></tr><tr><td>Exp COMMA <eq>Args_1</eq></td><td>t1 = new_temp()code1 = translate_Exp(Exp, sym_table, t1)arg_list = t1 + arg_listcode2 = translate_Args(<eq>Args_1</eq>, sym_table, arg_list)return code1 + code2</td></tr></table>

# 3.2.8 翻译模式（数组与结构体）

C--语言的数组实现采取的是最简单的 C 风格。数组和结构体不同于一般变量的一点在于，访问某个数组元素或结构体的某个域需要用到其内存地址的运算。以三维数组为例，假设有数组 int array[7][8][9]，为了访问数组元素 array[3][4][5]，我们首先需要找到三维数组 array 的首地址（直接对变量 array 取地址即可），然后找到二维数组 array[3] 的首地址（array 的地址加上 3 乘以二维数组的大小（8×9）再乘以 int 类型的宽度 4），然后找到一维数组 array[3][4] 的首地址（array[3] 的地址加上 4 乘以一维数组的大小（9）再乘以 int 类型的宽度 4），最后找到整数 array[3][4][5] 的地址（array[3][4] 的地址加上 5 乘以 int 类型的宽度 4）。整个运算过程可以表示为： 

$$
\begin{array}{r l} & \text {ADDR(array[i][j][k]) = ADDR(array) + \sum_ {t = 0} ^ {i - 1} SIZEOF(array[t]) + \sum_ {t = 0} ^ {j - 1} SIZEOF(array[i]} \\ & [ t ]) + \sum_ {t = 0} ^ {k - 1} S I Z E O F (a r r a y [ i ] [ j ]). \end{array}
$$

上式很容易推广到任意维数组的情况。 

结构体的访问方式与数组非常类似。例如，假设要访问结构体 struct {int x[10]; int y, z;} st 中的域 z，我们首先找到变量 st 的首地址，然后找到 st 中域 z 的首地址（st 的地址加上数组 x 的大小（4×10）再加上整数 y 的宽度 4）。我们可以把一个有 n 个域的 结构体看成为一个有 n 个元素的 “一维数组”，它与一般一维数组的不同点在于，一般一维数组的每个元素的大小都是相同的，而结构体的每个域大小可能不一样。其地址运算的过程可以表示为： 

$$
\mathrm{ADDR} (\text { st.field } _ {n}) = \mathrm{ADDR} (\text { st }) + \sum_ {t = 0} ^ {n - 1} \text { SIZEOF } (\text { st.field } _ {t}) 。
$$

将数组的地址运算和结构体的地址运算结合起来也并不是太难的事。假如我们有一个结构体，该结构体的某个元素是数组，为了访问该数组中的某个元素，我们需要先根据该数组在结构体中的位置定位到这个数组的首地址，然后再根据数组的下标定位该元素。反之，如果我们有一个数组，该数组的每个元素都是结构体。为了访问某个数组元素的某个域，我们需要先根据数组的下标定位要访问的结构体，再根据域的位置寻找要访问的内容。这个过程中唯一需要关注的是，我们应记录并区分在访问过程中使用到的临时变量哪些代表地址，哪些代表内存中的数值。如果弄错，会导致代码的运行结果出错或者非法的内存访问。当然，上述访问方式需要经历多次地址计算，如果我们能通过其他手段将这多次地址计算合并成一次，那么得到的中间代码的效率就会得到一定的提高。 

数组和结构体的翻译模式以及其他语法单元的翻译模式我们留给大家思考。 

# 3.2.9 中间代码生成提示

实验三需要你在实验二的基础上完成。你可以在实验二的语义分析部分添加中间代码的生成内容，使编译器可以一边进行语义检查一边生成中间代码；也可以将关于中间代码生成的所有内容写到一个单独的文件中，等到语义检查全部完成并通过之后再生成中间代码。前者会让你的编译器效率高一些，后者会让你的编译器模块性更好一些。 

确定了在哪里进行中间代码生成之后，下一步就要实现中间代码的数据结构（最好能写一系列可以直接生成一条中间代码的构造函数以简化后面的实现），然后按照输出格式的要求自己编写函数将你的中间代码打印出来。完成之后建议先自行写一个测试程序，在这个测试程序中使用构造函数人工构造一段代码并将其打印出来，然后用我们提供的虚拟机小程序简单地测试一下，以确保数据结构和打印函数都能正常工作。准备工作完成之后，再继续做下面的内容。 

接下来的任务是根据前面介绍的翻译模式完成一系列的 translate 函数。我们已经给出了 Exp 和 Stmt 的翻译模式，你还需要考虑包括数组、结构体、数组与结构体定义、变量初始化、语法单元 CompSt、语法单元 StmtList 在内的翻译模式。需要你考虑的内容实际上并不太多，最关键的一点在于一定要读懂前面介绍的几个 translate 函数 的意思。如果读懂了，那么无论是将它们实现到你的编译器中还是写新的translate函数，或者对这些translate函数进行改进都将是比较容易的。如果没读懂，例如没想明白某些translate函数中出现的place参数究竟有什么用，那么建议你还是先别急着动手写代码。如果顺利完成了所有translate函数，并将其连同中间代码的打印函数添加到了你的编译器中，那么实验三的要求就差不多完成了。建议在这里多写几份测试用例检查你的编译器，如果发现了错误需要及时纠正。 

最后，虚拟机小程序将以总共执行过的中间代码条数为标准来衡量你的编译器所输出的中间代码的运行效率。因此如果要进行代码优化，重点应该放在精简代码逻辑以及消除代码冗余上。 