## lab1 实验报告
学号:PB19151776 姓名:廖佳怡

## 实验要求
* 词法分析
  根据`Cminus-f`的词法补全`src/parser/lexical_analyzer.l`文件，完成词法分析器。其中，需补全模式和动作，即识别出`token`,`text`,`line`,`pos_start`,`pos_end`，一部分`token`要传递给语法分析器，一部分`token`要过滤掉。
* 语法分析
  根据`Cminus-f`的语法补全`src/parser/syntax_analyzer.y`文件，完成语法分析器，使之能从`Cminus-f`代码得到一颗语法树。其中，需按照文法填写相应的规则。

## 实验难点
* 理解`Cminus-f`的词法和语法规则
* 词法分析
  * 理解`Flex`的规则和文件布局
  * 注释`COMMENT`的正则表达式匹配，及其`line`,`pos_start`,`pos_end`的计算
  * 需过滤掉的字符如空格、换行，和ERROR的处理
* 语法分析
  * 理解`Bison`的规则和文件布局
  * 定义`tokens`的时候优先级和结合规则的处理
  * 理解`pass_node(), node(), and syntax_tree.h`
  * 定义语法规则时，语法树节点的构造
  * `EMPTY`的语法表示
## 实验设计
* 词法分析
  * 补全`flex`的模式和动作,涉及到关键字、专用符号、标识和整数、注释、以及空格、换行和错误等
  * `COMMENT`的模式为:
    ```
    char      [^\*/]
    COMMENT   \/\*{char}*((\*)+{char}+|\/{char}*)*(\*)*\*\/
    ```
    动作为:
    ```
    {
        pos_start = pos_end;
        int i;
        for(i = 0; yytext[i] != '\0'; i++)
        {
            pos_end++;
            if(yytext[i] == '\n')
            {
                lines++;
                pos_end = 1;
            }
        }
        pass_node(yytext); 
    }
    ```
  * 需要过滤掉的`token`在动作中不用return给语法分析器
* 语法分析
  * 补全union：
  ```
    %union {
        struct _syntax_tree_node* node;
        }
  ```
  * 补全其它声明
  * 声明时的优先级定义：
  ```
        %nonassoc LOWER_THAN_ELSE
        %nonassoc ELSE
        %right ASSIGN
        %left EQ NEQ
        %left LT LTE GT GTE
        %left ADD SUB
        %left MUL DIV
        %left LPARENTHESE RPARENTHESE LBRACKET RBRACKET LBRACE RBRACE
  ```
  * 构建语法规则
  * `if-else`二义性处理：
  ```
    selection_stmt: IF LPARENTHESE expression RPARENTHESE statement %prec LOWER_THAN_ELSE{$$ = node( "selection-stmt", 5, $1, $2, $3, $4, $5);}
            | IF LPARENTHESE expression RPARENTHESE statement ELSE statement {$$ = node( "selection-stmt", 7, $1, $2, $3, $4, $5, $6, $7);};
  ```
## 实验结果验证
请提供部分自行设计的测试
##### 测试样例如下：
```
  float ljy(float a, float b)
  {
      int c;/***
  comment test...*//**/c = 1;
      if( c == 1 )
          if( a >= b ) 
              return a; 
          else
              return b;
  }
```
##### 构建的语法树如下：
```
|  >--+ declaration-list
|  |  >--+ declaration
|  |  |  >--+ fun-declaration
|  |  |  |  >--+ type-specifier
|  |  |  |  |  >--* float
|  |  |  |  >--* ljy
|  |  |  |  >--* (
|  |  |  |  >--+ params
|  |  |  |  |  >--+ param-list
|  |  |  |  |  |  >--+ param-list
|  |  |  |  |  |  |  >--+ param
|  |  |  |  |  |  |  |  >--+ type-specifier
|  |  |  |  |  |  |  |  |  >--* float
|  |  |  |  |  |  |  |  >--* a
|  |  |  |  |  |  >--* ,
|  |  |  |  |  |  >--+ param
|  |  |  |  |  |  |  >--+ type-specifier
|  |  |  |  |  |  |  |  >--* float
|  |  |  |  |  |  |  >--* b
|  |  |  |  >--* )
|  |  |  |  >--+ compound-stmt
|  |  |  |  |  >--* {
|  |  |  |  |  >--+ local-declarations
|  |  |  |  |  |  >--+ local-declarations
|  |  |  |  |  |  |  >--* epsilon
|  |  |  |  |  |  >--+ var-declaration
|  |  |  |  |  |  |  >--+ type-specifier
|  |  |  |  |  |  |  |  >--* int
|  |  |  |  |  |  |  >--* c
|  |  |  |  |  |  |  >--* ;
|  |  |  |  |  >--+ statement-list
|  |  |  |  |  |  >--+ statement-list
|  |  |  |  |  |  |  >--+ statement-list
|  |  |  |  |  |  |  |  >--* epsilon
|  |  |  |  |  |  |  >--+ statement
|  |  |  |  |  |  |  |  >--+ expression-stmt
|  |  |  |  |  |  |  |  |  >--+ expression
|  |  |  |  |  |  |  |  |  |  >--+ var
|  |  |  |  |  |  |  |  |  |  |  >--* c
|  |  |  |  |  |  |  |  |  |  >--* =
|  |  |  |  |  |  |  |  |  |  >--+ expression
|  |  |  |  |  |  |  |  |  |  |  >--+ simple-expression
|  |  |  |  |  |  |  |  |  |  |  |  >--+ additive-expression
|  |  |  |  |  |  |  |  |  |  |  |  |  >--+ term
|  |  |  |  |  |  |  |  |  |  |  |  |  |  >--+ factor
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  >--+ integer
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  >--* 1
|  |  |  |  |  |  |  |  |  >--* ;
|  |  |  |  |  |  >--+ statement
|  |  |  |  |  |  |  >--+ selection-stmt
|  |  |  |  |  |  |  |  >--* if
|  |  |  |  |  |  |  |  >--* (
|  |  |  |  |  |  |  |  >--+ expression
|  |  |  |  |  |  |  |  |  >--+ simple-expression
|  |  |  |  |  |  |  |  |  |  >--+ additive-expression
|  |  |  |  |  |  |  |  |  |  |  >--+ term
|  |  |  |  |  |  |  |  |  |  |  |  >--+ factor
|  |  |  |  |  |  |  |  |  |  |  |  |  >--+ var
|  |  |  |  |  |  |  |  |  |  |  |  |  |  >--* c
|  |  |  |  |  |  |  |  |  |  >--+ relop
|  |  |  |  |  |  |  |  |  |  |  >--* ==
|  |  |  |  |  |  |  |  |  |  >--+ additive-expression
|  |  |  |  |  |  |  |  |  |  |  >--+ term
|  |  |  |  |  |  |  |  |  |  |  |  >--+ factor
|  |  |  |  |  |  |  |  |  |  |  |  |  >--+ integer
|  |  |  |  |  |  |  |  |  |  |  |  |  |  >--* 1
|  |  |  |  |  |  |  |  >--* )
|  |  |  |  |  |  |  |  >--+ statement
|  |  |  |  |  |  |  |  |  >--+ selection-stmt
|  |  |  |  |  |  |  |  |  |  >--* if
|  |  |  |  |  |  |  |  |  |  >--* (
|  |  |  |  |  |  |  |  |  |  >--+ expression
|  |  |  |  |  |  |  |  |  |  |  >--+ simple-expression
|  |  |  |  |  |  |  |  |  |  |  |  >--+ additive-expression
|  |  |  |  |  |  |  |  |  |  |  |  |  >--+ term
|  |  |  |  |  |  |  |  |  |  |  |  |  |  >--+ factor
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  >--+ var
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  >--* a
|  |  |  |  |  |  |  |  |  |  |  |  >--+ relop
|  |  |  |  |  |  |  |  |  |  |  |  |  >--* >=
|  |  |  |  |  |  |  |  |  |  |  |  >--+ additive-expression
|  |  |  |  |  |  |  |  |  |  |  |  |  >--+ term
|  |  |  |  |  |  |  |  |  |  |  |  |  |  >--+ factor
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  >--+ var
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  >--* b
|  |  |  |  |  |  |  |  |  |  >--* )
|  |  |  |  |  |  |  |  |  |  >--+ statement
|  |  |  |  |  |  |  |  |  |  |  >--+ return-stmt
|  |  |  |  |  |  |  |  |  |  |  |  >--* return
|  |  |  |  |  |  |  |  |  |  |  |  >--+ expression
|  |  |  |  |  |  |  |  |  |  |  |  |  >--+ simple-expression
|  |  |  |  |  |  |  |  |  |  |  |  |  |  >--+ additive-expression
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  >--+ term
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  >--+ factor
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  >--+ var
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  >--* a
|  |  |  |  |  |  |  |  |  |  |  |  >--* ;
|  |  |  |  |  |  |  |  |  |  >--* else
|  |  |  |  |  |  |  |  |  |  >--+ statement
|  |  |  |  |  |  |  |  |  |  |  >--+ return-stmt
|  |  |  |  |  |  |  |  |  |  |  |  >--* return
|  |  |  |  |  |  |  |  |  |  |  |  >--+ expression
|  |  |  |  |  |  |  |  |  |  |  |  |  >--+ simple-expression
|  |  |  |  |  |  |  |  |  |  |  |  |  |  >--+ additive-expression
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  >--+ term
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  >--+ factor
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  >--+ var
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  >--* b
|  |  |  |  |  |  |  |  |  |  |  |  >--* ;
|  |  |  |  |  >--* }
```
## 实验反馈
刚看到两个实验文档和代码框架的时候有点不知所措，摸索了一个晚上也只是了解个大概，甚是害怕。第二天仔细琢磨了一下代码，查了很多Flex和Bison的资料，突然悟了。写起来挺顺畅的。只遇到了node命名不正确，COMMENT识别不出来，树根节点搞错，几个bug，很快发现并修正，pass掉了助教给的三个test。
后来自己写了个test bench测试if-else二义性问题，发现按照文档给的语法，if-if-else会将else与第一个if匹配，但实际上应该与第二个if匹配，于是又增加了优先级处理。
总体感觉实验设计得非常合理，让我掌握了很多新知识，但又不至于做不出来。