 ASTParser for VSL
 =====================

## 运行方式：
**1.控制台输入任意表达式**
如：A + B * C  
生成表达式AST  
解析输出：Parsed a top-level expr  
然后将表达式AST的父子，兄弟节点关系用缩进形式表达输出  

**2.控制台输入函数**
如：FUNC main(a, b, c) {a + b * c}  
生成函数定义AST  
解析输出：Parsed a function definition  
然后将函数定义AST的父子，兄弟节点关系用缩进形式表达输出  

#### 注：本次实验中函数body暂时用一个简单表达式表示，随着实验深入而不断改进。
