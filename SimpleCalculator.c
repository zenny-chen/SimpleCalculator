//
//  main.c
//  SimpleCalculator
//
//  Created by Zenny Chen on 2016/12/20.
//  Copyright © 2016年 CodeLearning Studio. All rights reserved.
//

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

/** 我们这里使用简约的var作为对象类型的自动推导 */
#define var     __auto_type

/** 我们指定输入表达式的最大长度为2047字节，超出部分将被截断 */
#define MAX_ARGUMENT_LENGTH     2047

/** 用于标记解析符号时的当前状态 */
enum PARSE_PHASE_STATUS
{
    PARSE_PHASE_STATUS_LEFT_OPERAND = 0,
    PARSE_PHASE_STATUS_RIGHT_OPERAND,
    PARSE_PHASE_STATUS_LEFT_PARENTHESIS,
    PARSE_PHASE_STATUS_NEED_OPERATOR = 4,
    PARSE_PHASE_STATUS_HAS_NEG = 8
};

/** 当前算术计算优先级 */
enum OPERATOR_PRIORITY
{
    /** 加减法优先级 */
    OPERATOR_PRIORITY_ADD,
    
    /** 乘除以及求模优先级 */
    OPERATOR_PRIORITY_MUL
};

/** 判定当前字符是否属于数字 */
static inline bool IsDigital(char ch)
{
    return ch >= '0' && ch <= '9';
}

/** 对数字进行解析 */
static double ParseDigital(const char *cursor, int *pRetLength)
{
    char value[MAX_ARGUMENT_LENGTH + 1];
    
    char ch;
    var index = 0;
    var hasDot = false;
    
    do
    {
        ch = cursor[index];
        if(ch == '.')
        {
            // 如果之前已经出现了小数点，那么这里就中断解析
            if(hasDot)
                break;
            else
            {
                hasDot = true;
                value[index] = ch;
            }
        }
        else if(!IsDigital(ch))
            break;  // 在其余情况下，倘若不是数字，则立即中断解析
        
        value[index++] = ch;
    }
    while(ch != '\0');
    
    if(pRetLength != NULL)
        *pRetLength = index;
    
    // atof函数是将一个字符数组中的内容转为一个double类型的浮点数值
    // 该函数在<stdlib.h>头文件中
    return atof(value);
}

static double AddOp(double a, double b)
{
    return a + b;
}

static double MinusOp(double a, double b)
{
    return a - b;
}

static double MulOp(double a, double b)
{
    return a * b;
}

static double DivOp(double a, double b)
{
    return a / b;
}

static double ModOp(double a, double b)
{
    // 由于求模操作时，操作数必须是整数，
    // 所以我们这里将a与b都转换为带符号的64位整数类型
    return (int64_t)a % (int64_t)b;
}

/** 定义了一个操作函数表，方便快速定位当前操作符所对应的操作函数 */
static double (* const opFuncTables[])(double, double) = {
    // 为了进一步节省全局存储空间，我们这里将根据ASCII码表找出最小的字符值，
    // 将该值作为0，后续的都减去该值。通过ASCII表可以知道，值最小的符号是%，
    // 它的值为0x25。然后我们可以用指定索引的初始化器对opFuncTables进行初始化
    ['%' - '%'] = &ModOp,
    ['*' - '%'] = &MulOp,
    ['+' - '%'] = &AddOp,
    ['-' - '%'] = &MinusOp,
    ['/' - '%'] = &DivOp,
};

/** 判定是否为有效操作符 */
static inline bool IsOperator(char ch)
{
    return ch >= '%' && ch <= '/';
}

/** 
 * 解析当前的算术表达式
 * @param ppCursor 指向当前算术表达式字符串的地址。
 * 它既是输入又是输出。当当前算术表达式作为括号进行计算时，
 * 需要将右括号的位置输出到实参。
 * @param leftOperand 当前左操作数的值
 * @param status 当前计算状态
 * @param priority 当前计算的算术优先级
 * @param pStatus 输出解析状态
 * @return 输出计算表达式的结果
*/
static double ParseArithmeticExpression(const char **ppCursor, double leftOperand, enum PARSE_PHASE_STATUS status, enum OPERATOR_PRIORITY priority, bool *pStatus)
{
    const char *cursor = *ppCursor;
    var rightOperand = 0.0;
    
    var length = 0;
    double (*pFunc)(double, double) = NULL;
    bool isSuccessful = true;
    
    char ch;
    
    do
    {
        ch = *cursor;
        
        // 先判定当前字符是否属于数字
        if(IsDigital(ch))
        {
            double value = ParseDigital(cursor, &length);
            cursor += length;
            
            if((status & PARSE_PHASE_STATUS_RIGHT_OPERAND) == PARSE_PHASE_STATUS_LEFT_OPERAND)
            {
                leftOperand = value;
                // 如果具有负数符号，则将左操作数做取相反数操作
                if((status & PARSE_PHASE_STATUS_HAS_NEG) != 0)
                    leftOperand = -leftOperand;
            }
            else
            {
                // 对于当前为右操作数的情况，根据操作符计算优先级，
                // 需要进一步判定后面的操作优先级是否大于前面的，
                // 如果大于前面的，则需要做递归计算
                rightOperand = value;
                // 如果具有负数符号，则将左操作数做取相反数操作
                if((status & PARSE_PHASE_STATUS_HAS_NEG) != 0)
                    rightOperand = -rightOperand;
            }
            
            // 清除负数标志
            status &= ~PARSE_PHASE_STATUS_HAS_NEG;
            
            // 添加后续需要算术操作符的状态标志
            status |= PARSE_PHASE_STATUS_NEED_OPERATOR;
        }
        else if(IsOperator(ch))
        {
            // 这个区间范围内包含了常用的算术操作符以及左右圆括号，
            // 因此我们在这个分支中同时对这两类符号进行解析判断
            if(ch == '(')
            {
                cursor++;
                
                if((status & PARSE_PHASE_STATUS_RIGHT_OPERAND) == 0)
                {
                    // 如果当前状态为左操作数
                    leftOperand = ParseArithmeticExpression(&cursor, 0.0, PARSE_PHASE_STATUS_LEFT_OPERAND | PARSE_PHASE_STATUS_LEFT_PARENTHESIS, OPERATOR_PRIORITY_ADD, &isSuccessful);
                }
                else
                {
                    // 如果当前状态为右操作数
                    rightOperand = ParseArithmeticExpression(&cursor, 0.0, PARSE_PHASE_STATUS_LEFT_OPERAND | PARSE_PHASE_STATUS_LEFT_PARENTHESIS, OPERATOR_PRIORITY_ADD, &isSuccessful);
                }
                // 如果当前游标所指向的字符不是')'，说明没有匹配到合适的)，中断解析
                if(!isSuccessful || *cursor != ')')
                {
                    isSuccessful = false;
                    break;
                }
                // 清除当前左括号状态
                status &= ~PARSE_PHASE_STATUS_LEFT_PARENTHESIS;
                
                // 添加后续需要算术操作符的状态标志
                status |= PARSE_PHASE_STATUS_NEED_OPERATOR;
            }
            else if(ch == ')')
            {
                // 如果遇到')'字符，说明需要与之前的'('进行匹配
                if((status & PARSE_PHASE_STATUS_LEFT_PARENTHESIS) == 0)
                {
                    // 如果当前不处于左括号状态，则说明这个是多余的)，立即中断解析
                    *pStatus = false;
                    return 0.0;
                }
                // 将当前游标位置输出给实参
                *ppCursor = cursor;
                
                return (pFunc != NULL)? pFunc(leftOperand, rightOperand) : leftOperand;
            }
            else
            {
                if((status & PARSE_PHASE_STATUS_NEED_OPERATOR) == 0)
                {
                    if(ch == '-')
                    {
                        // 如果当前不需要操作符，则将减号视作为负数符号
                        // 作为负数符号的话后面必须跟一个数，否则也是无效的
                        if(IsDigital(cursor[1]))
                           status |= PARSE_PHASE_STATUS_HAS_NEG;
                        else
                        {
                            isSuccessful = false;
                            break;
                        }
                    }
                    else
                    {
                        // 对于其他情况，如果当前状态不需要操作符，那么表达式非法，立即中断解析
                        isSuccessful = false;
                        break;
                    }
                }
                else
                {
                    var tmpFunc = opFuncTables[ch - '%'];
                    if(tmpFunc == NULL)
                    {
                        // 如果没找到对应的操纵符函数，说明当前输入字符是非法的，直接中断解析
                        isSuccessful = false;
                        break;
                    }
                    var pry = (tmpFunc == AddOp || tmpFunc == MinusOp)? OPERATOR_PRIORITY_ADD : OPERATOR_PRIORITY_MUL;
                    
                    if(pFunc == NULL)
                        pFunc = tmpFunc;
                    
                    if((status & PARSE_PHASE_STATUS_RIGHT_OPERAND) == 0)
                    {
                        // 当前操作符解析成功，后续将需要该操作的右操作数
                        status |= PARSE_PHASE_STATUS_RIGHT_OPERAND;
                    }
                    else
                    {
                        // 如果之前优先级不小于当前操作符的优先级，那么立即做归约
                        if(priority >= pry)
                        {
                            leftOperand = pFunc(leftOperand, rightOperand);
                            rightOperand = 0.0;
                            // 随后更新当前操作函数以及计算优先级
                            pFunc = tmpFunc;
                        }
                        else
                        {
                            // 如果当前碰到了比之前优先级更改的操作符，
                            // 那么我们采用递归的方式进行计算
                            if(pFunc == MinusOp)
                            {
                                // 如果之前的计算是减法，那么根据减法不适用于结合律的性质，
                                // 我们这里将它作为一个加法，并且将右操作数取负
                                pFunc = AddOp;
                                rightOperand = -rightOperand;
                            }
                            var value = ParseArithmeticExpression(&cursor, rightOperand, PARSE_PHASE_STATUS_LEFT_OPERAND | PARSE_PHASE_STATUS_NEED_OPERATOR, pry, pStatus);
                            return pFunc(leftOperand, value);
                        }
                    }
                    // 更新当前计算优先级
                    priority = pry;
                }
                // 清除需要操作符标志
                status &= ~PARSE_PHASE_STATUS_NEED_OPERATOR;
            }
            
            cursor++;
        }
        else
            cursor++;
    }
    while(ch != '\0');
    
    // 若解析失败，则后续出结果时不做任何相关计算
    if(!isSuccessful)
        pFunc = NULL;
    
    if(pStatus != NULL)
        *pStatus = isSuccessful;
    
    return (pFunc == NULL)? leftOperand : pFunc(leftOperand, rightOperand);
}

/**
 * 计算输入的算术表达式
 * @param expr 输入的算术表达式字符串
 * @param result 以字符串的形式输出结果，这里设置了实参至少需要提供的缓存长度
 * @return 如果表达式解析成功，返回true，否则返回false
*/
bool CalculateArithmeticExpression(const char expr[], char result[static 32])
{
    if(expr[0] == '\0')
        return false;
    
    bool ret;
    
    var value = ParseArithmeticExpression(&expr, 0.0, PARSE_PHASE_STATUS_LEFT_OPERAND, OPERATOR_PRIORITY_ADD, &ret);
    
    if(!ret)
        return ret;
    
    sprintf(result, "%f", value);
    
    // 我们下面将把多余的.00000这种字样给过滤掉，使得结果输出更好看一些
    int length = (int)strlen(result);
    
    var dotIndex = -1;
    var hasE = false;
    for(int i = 0; i < length; i++)
    {
        if(result[i] == '.')
            dotIndex = i;
        else if(result[i] == 'e')
            hasE = true;
    }
    // 我们只有在仅存在小数点的情况下做过滤
    if(dotIndex >= 0 && !hasE)
    {
        var index = length;
        
        while(--index > 0)
        {
            if(result[index] != '0')
                break;
            
            result[index] = '\0';
        }
        // 如果dotIndex后面没有具体数字了，那么我们将小数点也过滤掉
        if(result[dotIndex + 1] == '\0')
            result[dotIndex] = '\0';
    }
    
    return true;
}


int main(int argc, const char * argv[])
{
    // argc用于存放参数个数。在Windows以及各类Unix系统中，参数以空格符进行分隔。
    // 所以我们在使用该程序时，计算表达式中不应该含有空格符（包括空格和制表符）。
    // 假定我们生成的可执行程序名为SimpleCalculator，那么在控制台输入：
    // SimpleCalculator 1+2
    // 是合法的。而输入：
    // SimpleCalculator 1 + 2
    // 则直接输出结果1，后面的+2会被忽略。
    if(argc < 2)
    {
        
        puts("No expression to calculate!");
        return 0;
    }
    
    var length = strlen(argv[1]);
    if(length == 0)
    {
        puts("No expression to calculate!");
        return 0;
    }
    
    // 对参数表达式长度做截断，取由宏指定的长度
    if(length > MAX_ARGUMENT_LENGTH)
        length = MAX_ARGUMENT_LENGTH;
    
    // 我们准备一个字符串缓存，将通过程序参数传递进来的字符串表达式拷贝到当前程序的栈上，
    // 便于后续解析操作
    char argBuffer[MAX_ARGUMENT_LENGTH + 1];
    
    // 我们这里使用strncpy也使得在做字符串拷贝的时候确保长度不超过指定的length大小。
    // 这里之所以使用strncpy而不是memcpy，
    // 是因为strncpy会在目标缓存最后添加一个字符串结束符'\0'
    // 该函数在<string.h>头文件中
    strncpy(argBuffer, argv[1], length);
    
    // 由于一些命令控制台不支持带有圆括号()的表达式，但支持方括号[]表达式，
    // 所以我们这里可以将输入中的[]再替换回()。
    for(typeof(length) i = 0; i < length; i++)
    {
        if(argBuffer[i] == '[')
            argBuffer[i] = '(';
        else if(argBuffer[i] == ']')
            argBuffer[i] = ')';
    }
    
    printf("The arithmetic expression to be calculated: %s\n", argBuffer);
    
    char result[32];
    
    if(CalculateArithmeticExpression(argBuffer, result))
        printf("The answer is: %s\n", result);
    else
        puts("Invalid expression!");
}
