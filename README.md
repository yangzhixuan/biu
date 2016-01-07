编译实习：Biu程序设计语言
=======

## 目录
[TOC]

##节一　介绍

在本次编译实习课程中，我们小组设计并实现了Biu这样一门静态类型的函数式编程语言及其编译器。在本文中我们详细描述我们对语言及其编译器的设计，以作本门课程之总结。

我们设计的Biu主要受Scheme、ML语言等函数式语言的影响，Biu的设计理念为“**保持直觉**（*Keep Things Intuitive*）” 。Biu的主要特性如下：

* **静态类型**：类型检查可以帮助程序员尽量早的找到代码中的错误并帮助程序员保持思路清晰。类型信息也能帮助编译器生成效率更高的代码。对于执行一次程序开销很大的情况（如科学计算），类型检查是非常有用的工具。
* **函数式**：大量的实践表明以函数为抽象手段在很多情况下是非常强大的。Biu被设计为一个完整的函数式语言，函数完全作为语言中的“一等公民”，能被当作值被返回或传递。
* **允许可变数据**：变化是真实世界的一大本质。可变数据（也就是副作用）在编程中的重要性甚至比函数更大，因此我们不把Biu设计为一门“纯函数式”语言，而是在Biu中允许可变数据的存在。但同时我们也把可变动的数据与不可变动的数据严格区分开。

我们也为Biu实现了一个编译器biuc。biuc基于LLVM框架，将biu编译为可执行的机器代码。如今，程序语言的效率越发重要以支持开发越来越复杂的软件。与此同时，现代编译器框架又给编译器带来了极好的可移植性。因此我们认为编译器可能是比解释器更好的选择，所以我们在本门课程中优先实现Biu的编译器。

接下来本文按如下顺序组织：

* 第二节详细讨论Biu的语法及其语言特性
* 第三节详细讨论编译器biuc的实现
* 在第四节中我们讨论下一步应该完成的工作及我们在本次课程中的结论。

##节二　Biu语言
###0. 语法
Biu采用Lisp的S表达式作为语法，具体来说便是将所有语言构造用括号括起来(称之为一个**form**)，括号中用空格分隔的各个元素表示这个构造的成分，比如`(+ 1 2)`是一次函数应用，被调用的函数是`+`，两个参数分别是1和2。S表达式当然是可以嵌套的，如`(if (< a b) a b)`。S表达式粗看很不自然，与人类的自然语言迥异。事实上，S表达式的思想是显式地表示出代码的语法解析树，将代码的结构从视觉上直接表现出来（程序语言代码中的缩进也是为了这个目的），这使得程序员能最清晰地看出代码的结构。S表达式还可以带来其他的诸多好处，比如S表达式配合一个强大的编辑器(如emacs的paredit模式)之后，程序员对代码的编辑就不在是对字符流进行编辑，而是对语法树进行编辑，可以很大地提高代码的编写与重构效率。

###1. 程序结构
当前的版本的实现中，暂只支持单文件形式的编译。一个`.biu`文件是一个Biu程序，`.biu`文件含有任意多个的form，逻辑上一个文件按从上到下的顺序执行这些form。

###2. 基本类型
当前的Biu实现中，支持了Number，Bool, Char这三种基本数据类型。用`define`可以定义这些类型的变量，如`(define a 5)`或`(define b true)`以及`(define c 'c')`，注意我们无需给出变量的类型，变量的类型是通过初始值推出的。在当前的实现中，Number被实现为64位的浮点数，Bool和Char则被实现为8位整数。

###3. 复合类型
复合类型指的是通过其他类型构造起来的类型。当前的Biu实现中，支持了数组及函数这两种复合类型。`T`类型元素的数组的类型是`(Array T)`，Biu采用`(make-array T n)`的形式创建一个n个T类型元素的数组。分别采用`(get array index)`及`(set! array index value)`的方式获取和修改数组中的元素（注意，元素可修改的数组为Biu带来了可以变动的数据的支持）。

```scheme
;;; example 2.1
(define (fib (n Number)
             (arr (Array Number)))
  (set! arr 0 0)
  (set! arr 1 1)

  (define ((loop Number) (idx Number))
    (define (update)
      (set! arr idx (+ (get arr (- idx 1))
                       (get arr (- idx 2))))
      (loop (+ idx 1)))
    (if (= idx n)
      (get arr (- n 1))
      (update)))

  (loop 2))

(define a (make-array Number 10))
(fib 10 a)
(print (get a 9))
```
上面的例子2.1中，我们定义了函数fib，其功能是求Fibonacci数列的每一项并储存到一个数组里。我们会在2.5小节中详细说明Biu中的函数定义。然后我们定义数组Number型的数组a，并对a调用fib函数，最后我们输出a的下标为9的元素（Biu的下标从0开始）。这段代码会输出34。

函数类型也是复合类型的一种，如`(=> Number Number Bool)`表示接收两个Number，返回一个Bool的函数的类型。再比如`(=> (=> Number Number) Number Number)`，表示接收一个函数（该函数则是接收一个Number返回一个Number）及一个Number作为参数，返回一个Number的函数的类型。我们在2.5小节中详细说明Biu中的函数。

###4. 控制结构
循环和分支是编程语言中最常见的控制结构。拥有更丰富的控制结构往往能给程序员带来方便，但也可能造成代码结构的混乱。Biu在控制结构的添加上是谨慎的，目前的实现中支持了最简单的双分支if结构，而函数的递归能替代循环（例子2.1中演示了这两种结构）。在本门课程后的Biu的下一步开发中，提供多分支的if（类似于Scheme的cond）、简单的循环结构是必要的。而异常、co-routine、call-with-current-continuation的添加则需要更谨慎的决定。

###5. 函数
Biu是一门函数式编程语言，函数是Biu最重要的特性。定义函数的基本语法是：

```scheme
(define ((函数名 返回类型) (参数名 参数类型)...)
  函数体)
  
;;; 对于非递归函数，返回类型可以省略
(define (函数名 (参数名 参数类型)...)
  函数体)
```
其中每一个函数参数必须给定类型，而函数返回值的类型则可以省略（如果省略返回值类型，函数的返回类型将根据函数体的返回类型确定）。函数参数的类型信息被编译器用于进行类型检查，编译器要求每次函数调用的实际参数的类型必须与函数定义时给定的参数类型一致。

函数的定义可以是嵌套的，也就是说函数体中可以继续定义函数。在Biu中变量的作用域是**词法作用域**，也就是在某个函数中能看到的变量是本函数的参数以及定义这个函数时所在的位置能看到的变量，而与调用这个函数的位置时能看到的变量无关。


```scheme
;;; example 2.2
(define (derivative (f (=> Number Number)))
  (define delta 0.000001)
  (define (d (x Number))
    (/ (- (f (+ x delta)) 
          (f (- x delta)))
       (+ delta delta)))
  d)

(define (poly (x Number))
  (+ x (* x x)))

(define poly-d (derivative poly))

(print (poly-d 5))

```
定义好的函数可以被当作返回值返回，也可当作参数进行传递。在上面的例子2.2中，derivative函数接收一个函数f，f是接收Number且返回Number的一个函数，而derivative返回的也是一个函数，该函数是f的近似的导函数。而poly则是表示 $x^2+x$ 的函数，通过`(derivative poly)`我们得到函数`poly-d`，上面的例子最后会输出`11`，也就是 $x^2+x$ 的导函数 $2x+1$ 在5处的值。 

###6. 库函数
吸引大家广泛使用一门语言的往往并不是这门语言的语言特性，而是这门语言的库。为了能让Biu能有一个易用的库，我们作的设计是支持Biu直接调用C语言程序。只需在C语言代码中将一个函数包装成Biu的函数底层实现的形式，然后在Biu中用extern-raw特殊形式声明这个函数，接下来的代码中就可以直接调用该函数了。链接时则需要将C语言代码与Biu代码一起链接。

举例来说，现在的Biu中对Number的加减乘除实际上都是实现为C函数的，如下：

```c
typedef struct closureType{
    void *func;
    void *env;
} closure;

static double add_func(void* env, double a, double b)
{
    return a+b;
}
closure add = {add_func, (void*)0};

```
然后在Biu中：

```scheme
(extern-raw + (=> Number Number Number) "add")
(+ 1 2)
```
链接时再将C代码的目标文件和Biu的目标文件一起链接即可。

###7. 为什么没有xxx?
1. 为什么没有自定义数据类型？
   
   因为自定义数据类型和类型系统有密切的关系，作者希望能更谨慎地选取自定义数据类型在Biu中出现的形式。可能的选择有：1. 面向对象风格的类/对象模型，2. C风格的简单结构体形式（Julia也采用类似的形式），3. Haskell风格的代数数据类型。目前我偏向于Haskell风格的代数数据类型。
   
2. 为什么没有多态/泛型？
   
   对于一门静态类型语言，parametric polymophism(或者说generics)几乎是一种必不可少的抽象方法。在接下来的开发中，泛型是最重要的目标。
   
3. 为什么没有xxx控制结构？

   Biu目前只有最简单的控制结构，的确不够方便。下一步的开发中我们将修饰这些控制结构使得Biu在更好用的同时也能保持简单性。至于复杂的控制结构如异常、协程、以及call/cc则需要谨慎考虑。

4. 会不会有类似Lisp的宏系统？

   宏系统对语言的扩展能力的确非常吸引人，然而宏系统也可能会造成程序员间的代码风格迥异、也会给语言的调试带来更大的困难。Biu暂不考虑加入宏系统。

###8. 一个完整的例子
本小节中我们查看一个稍大一些的Biu语言例子：一个[brainf**k](http://www.wikiwand.com/en/Brainfuck)(以下简称BF)语言的解释器。BF语言和图灵机的概念很相似，程序操纵一个以字节为单位、初始化为0的数组，以及指向这个数组的一个指针。而代码只包含八种字符，分别是：

| 指令 | 功能 |
|---|---|
| > | 指针加一|
| < | 指针减一|
| + | 指针指向的字节的值加一|
| - | 指针指向的字节的值减一|
| . | 输出指针指向的单元内容（ASCII码）|
| , | 输入内容到指针指向的单元（ASCII码）|
| [ | 如果指针指向的单元值为零，向后跳转到对应的]指令的次一指令处|
| ] | 如果指针指向的单元值不为零，向前跳转到对应的[指令的次一指令处|

我们的BF解释器代码的第一部分是对我们用到的库函数的声明，由于目前Biu还没有确立多文件编译方式，所以这些声明暂时还需要手工写入到代码中，日后则不需要写入。

```scheme
(extern-raw + (=> Number Number Number) "add")
(extern-raw - (=> Number Number Number) "sub")
(extern-raw * (=> Number Number Number) "mul")
(extern-raw / (=> Number Number Number) "__div")
(extern-raw sin (=> Number Number) "__sin")
(extern-raw = (=> Number Number Bool) "equal")
(extern-raw > (=> Number Number Bool) "greater")
(extern-raw < (=> Number Number Bool) "less")
(extern-raw char-equal? (=> Char Char Bool) "equal_char")
(extern-raw print (=> Number Number) "print")
(extern-raw cos (=> Number Number) "cosclosure")
(extern-raw sin (=> Number Number) "sinclosure")
(extern-raw getchar (=> Char) "getcharclosure")
(extern-raw putchar (=> Char Number) "putcharclosure")
(extern-raw number->char (=> Number Char) "numbertocharclosure")
(extern-raw char->number (=> Char Number) "chartonumberclosure")
```

接下来的`read_program`函数是从用户读入代码的函数。该函数用递归的方式来实现循环。

```scheme
(define ((read_program Number)
         (program (Array Char))
         (index Number))

  (define (save-char (c Char))
    (set! program index c)
    (read_program program (+ index 1)))

  (define c (getchar))

  (if (= (char->number c) -1)
    index
    (save-char c)))

(define program (make-array Char 10000))

(define length (read_program program 0))

;(print length)

```

然后则是interprete函数，它从前往后执行BF程序的每一条指令。从interprete函数中可以看出缺少多分支if语句带来的不方便性。

```scheme
(define ((interpret Bool)
         (program (Array Char)))

  (define memory (make-array Char 1000))

  (define ((loop Bool)
           (ptr Number)
           (index Number))

    (define (move (offset Number))
      (loop (+ ptr 1) (+ index offset)))

    (define (add (amout Number))
      (set! memory index
        (number->char (+ amout (char->number (get memory index)))))
      (loop (+ ptr 1) index))

    (define (read)
      (set! memory index (getchar))
      (loop (+ ptr 1) index))

    (define (output)
      (putchar (get memory index))
      (loop (+ ptr 1) index))

    (define ((findmatch Number) (ptr Number) (value Number) (offset Number))
      (if (= value 0)
        ptr
        (if (char-equal? (get program ptr) '[')
          (findmatch (+ ptr offset) (+ value 1) offset)
          (if (char-equal? (get program ptr) ']')
            (findmatch (+ ptr offset) (- value 1) offset)
            (findmatch (+ ptr offset) value offset)))))

    (define (while-enter)
      (if (= (char->number (get memory index)) 0)
        (loop (findmatch (+ ptr 1) 1 1) index)
        (loop (+ ptr 1) index)))

    (define (while-exit)
      (loop (+ (findmatch (- ptr 1) -1 -1) 1) index))

    (define c (get program ptr))

    ;(putchar c)

    (if (> ptr length)
      true
      (if (char-equal? c '>')
        (move 1)
        (if (char-equal? c '<')
          (move -1)
          (if (char-equal? c '+')
            (add 1)
            (if (char-equal? c '-')
              (add -1)
              (if (char-equal? c '[')
                (while-enter)
                (if (char-equal? c ']')
                  (while-exit)
                  (if (char-equal? c '.')
                    (output)
                    (if (char-equal? c ',')
                      (read)
                      false))))))))))
  (loop 0 0))


(interpret program)
```

##节三　biuc编译器

LLVM是一个较成熟且功能强大的编译器框架。我们只需要负责把Biu代码翻译为LLVM IR（一种带类型的底层中间表示），而机器代码的生成则交给LLVM的工具链即可。我们的编译器主要如下的三个模块：

1. 词法及语法分析
2. 类型检查
3. LLVM IR代码生成

本节接下来分别介绍这三个部分的设计及实现。

###1. 词法及语法分析
我们没有采用flex/bison这样的词法分析器生成器，而是选择手工实现词法分析与语法分析。一方面是觉得这样的分析器生成器生成的代码的可扩展性比较差，不方便后期添加更详细地报错信息以及与调试器集成；另一方面是觉得现有的分析器生成器都设计得不够模块化，并不方便使用。再加上Biu的语法实际上相当简单，所以我们选择了手工实现词法分析与语法分析。

####词法分析
词法分析器将代码文件的字符流转变成token流，我们定义了如下几种符号：

* TOK_NUMBER: 数字
* TOK_CHAR: 字符
* TOK_SYMBOL: 符号
* 其他字符

对于我们的几种符号及它们相应的词法规则，我们的词法分析器可以根据下一个字符预测出token是什么类型，然后进行相应的处理。但我们的词法器并非完全是这样LL(1)的，举例来说(下面的代码3.1)，如果下一个字符是负号或者数字的话，分析器会预测这个token是一个数字，于是它继续读这个数字的余下位数，当余下位数读完后若其发现这个token只有一个负号，它就会把这个负号当作一个TOK_SYMBOL处理。从这样的处理中便能看出手写分析器的便捷之处：我们不必受某种特定文法集合的限制，对于文法中简单的部分我们可以自行实现高效的分析方法，对于文法中复杂的部分我们则可以实现递归下降这样表达能力强大的分析方法。

```c
// example 3.1
    if(isdigit(lastChar) || lastChar == '-') {
        // number: -?[0-9]+(.[0-9]+)?
        string numStr = "";
        do {
            numStr += lastChar;
            lastChar = getchar();
        } while(isdigit(lastChar) || lastChar == '.');

        if(numStr == "-") {
            symbolStr = "-";
            return tok_symbol;
        }

        const char* endpoint;
        numVal = strtod(numStr.c_str(), nullptr);
        return tok_number;
    }
```

####语法分析
语法分析器的目的是将token流转变成语法树。S表达式实际上已经是树结构：括号中的各个元素就是该form的子节点。我们在这里还会将各种特殊形式（special form），比如`if-form`、`define-form`与一般的form(代表函数应用)区分开，因为它们的元素应该有固定的形式（特定的个数和类型）。在biuc的实现中，各种类型的语法分析树节点被实现为层次化的类。

我们的语法分析器是LL(1)的，它通过下一个token的类型就能判断现在应该使用哪一个产生式。举例来说，假设当前在解析一个form，那么第一个token应该是左括号，而第二个token若是`if`或`define`，则按照它们相应的规则进行余下的解析，而如果是其他符号，则说明是一般的函数应用，于是调用解析表达式的函数进行处理。如下代码3.2所示。

```c++
// code 3.2
unique_ptr<FormAST> parseForm()
{
    if(curTok != '(') {
        parserError(string("parseForm expects a '(', get: ") + tok2str(curTok));
        return nullptr;
    }
    getNextToken();

    if(curTok == tok_symbol && symbolStr == "if") {
        // if-form
        getNextToken();
        auto form = llvm::make_unique<IfFormAST>();
        form->condition = parseExpr();
        form->branch_true = parseExpr();
        form->branch_false = parseExpr();

        if(curTok != ')') {
            parserError(string("parseForm: if-form expect a ')', get: ") + tok2str(curTok));
            return nullptr;
        }
        getNextToken();
        return std::move(form);
    } else if ( ... ) {
        // branches for other special forms...
    } else {
        auto form = llvm::make_unique<ApplicationFormAST>();
        while(curTok != ')') {
            form->elements.push_back( parseExpr() );
        }
        getNextToken();
        return std::move(form);
    }
}
```
###2. 类型检查
类型检查是Biu编译中重要的一步。对于通过了类型检查的程序，我们往往可以保证程序具有某些好的性质，比如说我们可以断定在没有运行时错误的情况下对程序的求值总是可以不断进行（所谓的progress性质）以及程序在每一步求值后变量的类型不会改变（所谓的preservation性质）。后续的代码生成等步骤也可以得到简化，因为它们总是可以知道变量的类型信息，这也有助于代码生成器生成出高效的代码。

类型检查的实现与编程语言的解释器的实现相当相似，往往都是语法制导的，并在检查/解释过程中维护一个变量名到类型/值的映射（环境），根据当前被检查/解释的语句的类型执行相应的检查/求值，并可能创建新的环境然后在新的环境中继续检查/求值。

在biuc当中，对某个类型的form进行类型检查的代码被实现为这个form的虚函数，比如下面的代码3.3是对`if-form`的类型检查。在其中我们检查条件部分的类型，并要求其为Bool，然后检查两个分支的类型，要求其类型相一致，并把这作为整个if语句的类型。

```c++
// code 3.3
shared_ptr<BiuType> IfFormAST::checkType(TypeEnvironment &e) {
    auto t = condition->checkType(e);
    if(*t != *boolType) {
        throw(CheckerError("IfForm condition must be bool, got " + t->identifier));
        return nullptr;
    }
    TypeEnvironment newEnv1(e);
    TypeEnvironment newEnv2(e);
    auto b1 = branch_true->checkType(newEnv1);
    auto b2 = branch_false->checkType(newEnv2);
    if(*b1 != *b2) {
        throw(CheckerError("IfForm type of branches mismatch, got " + b1->identifier + " and " + b2->identifier));
        return nullptr;
    }
    return retType = b1;
}
```

###3. 代码生成
代码生成的目标是将已经通过类型检查的Biu程序翻译为LLVM IR这一中间表示。LLVM IR是一种较底层的中间表示，其采用了静态单赋值的形式。Biu是一门抽象层次较高的语言，代码生成的主要难点是将Biu中的高层结构在LLVM IR这种较底层的语言中实现。而其中最重要的则是Biu中的函数在LLVM IR中的实现。LLVM IR中的函数与C语言的函数类似，只能定义全局的函数。而Biu中的函数可以嵌套定义，且可以引用外层函数中的变量，也可以被当作返回值或参数传递，而且内层函数在被当作返回值传递后必须仍然能保持对它引用的外层变量的访问（称之为闭包）。

为了实现Biu中的函数，我们采用了一种称之为flat-closure的方法。所有的函数，不管是外层的还是内层的，都被编译为LLVM IR中的全局函数，且函数中引用的更外层函数的变量（称之为自由变量），都被转换为该函数的一个参数，该参数是由所有自由变量构成的结构体（称之为环境）。而Biu中的函数对象，则是由指向它LLVM IR中的函数的函数指针以及指向它环境的指针组成。当一个函数被定义时，它的环境被创建在堆上，环境中用到的变量（即它原来的自由变量）被拷贝到环境中。

下面是函数定义的代码生成的实现。其中第一步是把函数体编译为LLVM IR中的全局函数：

```cpp
Value* DefineFuncFormAST::codeGen(ValueEnvironment &e)
{
    // Create function
    // TODO: use a hashed unique name instead of the original name
    Function *F = Function::Create(flatFuncType, Function::ExternalLinkage,
            name->identifier, theModule.get());

    int idx = 0;
    for(auto & arg : F -> args()) {
        if(idx == 0){
            arg.setName("closure");
        } else {
            arg.setName(argList[idx].first->identifier);
            idx++;
        }
    }

    // Create Basic block
    BasicBlock *bb = BasicBlock::Create(theModule->getContext(), "entry", F);
    BasicBlock *oldBB = Builder.GetInsertBlock();
    Builder.SetInsertPoint(bb);
    ValueEnvironment newEnv;

    // Binds free variables (variables in closure)
    auto ite = F->arg_begin();
    auto strV = Builder.CreateLoad(ite);
    for(auto & freeVarIter : freeVarIndex) {
        //Value *addr = Builder.CreateStructGEP(ite, freeVarIter.second);
        //Value *val = Builder.CreateLoad(addr);
        auto val = Builder.CreateExtractValue(strV, { (unsigned) freeVarIter.second });
        newEnv[freeVarIter.first] = val;
    }

    // Binds function arguments
    ite++;
    int argIndex = 0;
    while(ite != F->arg_end()) {
        newEnv[argList[argIndex].first->identifier] = ite;
        ite++;
        argIndex++;
    }

    Value* last = nullptr;
    for(auto & b : body) {
        last = b->codeGen(newEnv);
    }
    Builder.CreateRet(last);
    verifyFunction(*F);
    if(oldBB) {
        Builder.SetInsertPoint(oldBB);
    }
```
    
紧接的下一步就是创建出函数对象，它是指向刚刚被生成出的函数指针以及指向环境的指针构成的对儿。

```cpp
    // Create the closure
    auto closure_v = Builder.CreateAlloca(funType->llvmType, nullptr, name->identifier);
    auto fp_v = Builder.CreateStructGEP(closure_v, 0);
    auto env_v = Builder.CreateStructGEP(closure_v, 1);
    Builder.CreateStore(F, fp_v);

    // allocate the enviroment in the heap
    Value * mallocSize = ConstantExpr::getSizeOf(envType);
    mallocSize = ConstantExpr::getTruncOrBitCast( cast<Constant>(mallocSize),
            Type::getInt64Ty(getGlobalContext()));
    Instruction * var_malloc= CallInst::CreateMalloc(Builder.GetInsertBlock(),
            Type::getInt64Ty(getGlobalContext()), envType, mallocSize);
    Builder.Insert(var_malloc);
    Value *var = var_malloc;
    Builder.CreateStore(var, env_v);

    // set the environment variables
    for(auto & freeVar : freeVarIndex) {
        auto ele_v = Builder.CreateStructGEP(var, freeVar.second);
        if(freeVar.first == name->identifier) {
            // a recursive function, this free variable refers the function itself
            auto fp_v_rec = Builder.CreateStructGEP(ele_v, 0);
            auto env_v_rec = Builder.CreateStructGEP(ele_v, 1);
            env_v_rec = Builder.CreateBitCast(env_v_rec, var->getType()->getPointerTo());
            fp_v_rec = Builder.CreateBitCast(fp_v_rec, F->getType()->getPointerTo());
            Builder.CreateStore(var, env_v_rec);
            Builder.CreateStore(F, fp_v_rec);
        } else {
            Builder.CreateStore(e[freeVar.first], ele_v);
        }
    }
    e[name->identifier] = Builder.CreateLoad(closure_v);
    return ConstantInt::getTrue(theModule->getContext());
}
```

###4. 接下来？

代码生成结束后，我们就可以用LLVM的工具链将生成LLVM IR编译为汇编代码，然后再用LLVM的汇编器进行机器代码的生成以及链接。对一个biu程序的完整编译过程如下：

```
$ ./biuc < filename
$ llc -O3 tmp.ll
$ clang tmp.s base.c array.c -o out
$ ./out
```

###5. 缺失的部分？
对当前版本的biuc，最缺失的功能应该是垃圾回收。实际上，我们对函数对象产生在堆上的数据没有进行任何回收处理。经典的引用计数形式的垃圾回收或者更高级的垃圾回收机制可以被添加到biuc之中。另外，尾递归优化对biu这样一门函数式编程语言来说也应该是必须要有的。


##节四　总结
在本学期的编译实习课程中，我们设计并实现了biu这样一门静态类型的函数式编程语言。在实践的过程中，最令我印象深刻的是语法制导过程在编译中的普遍性。无论是解释，还是类型检查或者是代码生成，全部都被实现为了结构相似的语法制导过程。不由让人联想起玩笑般的[格林斯潘第十定律](https://www.wikiwand.com/zh-cn/%E6%A0%BC%E6%9E%97%E6%96%AF%E6%BD%98%E7%AC%AC%E5%8D%81%E5%AE%9A%E5%BE%8B)：

> 任何C或Fortran程序复杂到一定程度之后，都会包含一个临时开发的、不合规范的、充满程序错误的、运行速度很慢的、只有一半功能的Common Lisp实现。

当前的Biu实现已经在小规模的任务中近乎可用了，但仍有许多必要的功能没有被实现。在未来的Biu开发工作中，如下几项任务是最重要的：

1. 泛型
2. 用户自定义类型
3. 垃圾回收