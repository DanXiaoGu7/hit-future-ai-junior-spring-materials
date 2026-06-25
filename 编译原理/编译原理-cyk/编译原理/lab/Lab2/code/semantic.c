#include "semantic.h"

pTable table;

// Type functions
pType newType(Kind kind, ...) { //构造一个新的类型结构体
    pType p = (pType)malloc(sizeof(Type));
    assert(p != NULL);
    p->kind = kind;
    va_list vaList;
    assert(kind == BASIC || kind == ARRAY || kind == STRUCTURE ||
           kind == FUNCTION);
    va_start(vaList, kind);
    switch (kind) {
        case BASIC:
            //va_start(vaList, 1);
            p->u.basic = va_arg(vaList, BasicType);
            break;
        case ARRAY:
            //va_start(vaList, 2);
            p->u.array.elem = va_arg(vaList, pType);
            p->u.array.size = va_arg(vaList, int);
            break;
        case STRUCTURE:
            //va_start(vaList, 2);
            p->u.structure.structName = va_arg(vaList, char*);
            p->u.structure.field = va_arg(vaList, pFieldList);
            break;
        case FUNCTION:
            //va_start(vaList, 3);
            p->u.function.argc = va_arg(vaList, int);
            p->u.function.argv = va_arg(vaList, pFieldList);
            p->u.function.returnType = va_arg(vaList, pType);
            break;
    }
    va_end(vaList);
    return p;
}

pType copyType(pType src) {
    if (src == NULL) return NULL;
    pType p = (pType)malloc(sizeof(Type));
    assert(p != NULL);
    p->kind = src->kind;
    assert(p->kind == BASIC || p->kind == ARRAY || p->kind == STRUCTURE ||
           p->kind == FUNCTION);
    switch (p->kind) {
        case BASIC:
            p->u.basic = src->u.basic;
            break;
        case ARRAY:
            p->u.array.elem = copyType(src->u.array.elem);
            p->u.array.size = src->u.array.size;
            break;
        case STRUCTURE:
            p->u.structure.structName = newString(src->u.structure.structName);
            p->u.structure.field = copyFieldList(src->u.structure.field);
            break;
        case FUNCTION:
            p->u.function.argc = src->u.function.argc;
            p->u.function.argv = copyFieldList(src->u.function.argv);
            p->u.function.returnType = copyType(src->u.function.returnType);
            break;
    }

    return p;
}

void deleteType(pType type) {
    assert(type != NULL);
    assert(type->kind == BASIC || type->kind == ARRAY ||
           type->kind == STRUCTURE || type->kind == FUNCTION);
    pFieldList temp = NULL;
    // pFieldList tDelete = NULL;
    switch (type->kind) {
        case BASIC:
            break;
        case ARRAY:
            deleteType(type->u.array.elem);
            type->u.array.elem = NULL;
            break;
        case STRUCTURE:
            if (type->u.structure.structName)
                free(type->u.structure.structName);
            type->u.structure.structName = NULL;

            temp = type->u.structure.field;
            while (temp) {
                pFieldList tDelete = temp;
                temp = temp->tail;
                deleteFieldList(tDelete);
            }
            type->u.structure.field = NULL;
            break;
        case FUNCTION:
            deleteType(type->u.function.returnType);
            type->u.function.returnType = NULL;
            temp = type->u.function.argv;
            while (temp) {
                pFieldList tDelete = temp;
                temp = temp->tail;
                deleteFieldList(tDelete);
            }
            type->u.function.argv = NULL;
            break;
    }
    free(type);
}


// true: 都为空
// false: 都为函数，两个kind不一样
boolean checkType(pType type1, pType type2) { //检查两个类型是否匹配，匹配->true/不匹配->false
    if (type1 == NULL || type2 == NULL) return TRUE;//有一个为空，匹配
    if (type1->kind == FUNCTION || type2->kind == FUNCTION) return FALSE;//函数不能参与比较
    if (type1->kind != type2->kind)//类型不一致一定不匹配
        return FALSE;
    else {
        assert(type1->kind == BASIC || type1->kind == ARRAY ||
               type1->kind == STRUCTURE);
        switch (type1->kind) {
            case BASIC:
                return type1->u.basic == type2->u.basic;
            case ARRAY:
                return checkType(type1->u.array.elem, type2->u.array.elem);//递归检查数组类型
            case STRUCTURE:
                return !strcmp(type1->u.structure.structName,
                               type2->u.structure.structName);//结构体：名等价
        }
    }
}

void printType(pType type) {
    if (type == NULL) {
        printf("type is NULL.\n");
    } else {
        printf("type kind: %d\n", type->kind);
        switch (type->kind) {
            case BASIC:
                printf("type basic: %d\n", type->u.basic);
                break;
            case ARRAY:
                printf("array size: %d\n", type->u.array.size);
                printType(type->u.array.elem);
                break;
            case STRUCTURE:
                if (!type->u.structure.structName)
                    printf("struct name is NULL\n");
                else {
                    printf("struct name is %s\n", type->u.structure.structName);
                }
                printFieldList(type->u.structure.field);
                break;
            case FUNCTION:
                printf("function argc is %d\n", type->u.function.argc);
                printf("function args:\n");
                printFieldList(type->u.function.argv);
                printf("function return type:\n");
                printType(type->u.function.returnType);
                break;
        }
    }
}

// FieldList functions
pFieldList newFieldList(char* newName, pType newType) { // 新的域节点
    pFieldList p = (pFieldList)malloc(sizeof(FieldList));
    assert(p != NULL);
    p->name = newString(newName);
    p->type = newType;
    p->tail = NULL;
    return p;
}

pFieldList copyFieldList(pFieldList src) {
    assert(src != NULL);
    pFieldList head = NULL, cur = NULL;
    pFieldList temp = src;

    while (temp) {
        if (!head) {
            head = newFieldList(temp->name, copyType(temp->type));
            cur = head;
            temp = temp->tail;
        } else {
            cur->tail = newFieldList(temp->name, copyType(temp->type));
            cur = cur->tail;
            temp = temp->tail;
        }
    }
    return head;
}

void deleteFieldList(pFieldList fieldList) {
    assert(fieldList != NULL);
    if (fieldList->name) {
        free(fieldList->name);
        fieldList->name = NULL;
    }
    if (fieldList->type) deleteType(fieldList->type);
    fieldList->type = NULL;
    free(fieldList);
}

void setFieldListName(pFieldList p, char* newName) {
    assert(p != NULL && newName != NULL);
    if (p->name != NULL) {
        free(p->name);
    }
    // int length = strlen(newName) + 1;
    // p->name = (char*)malloc(sizeof(char) * length);
    // strncpy(p->name, newName, length);
    p->name = newString(newName);
}

void printFieldList(pFieldList fieldList) {
    if (fieldList == NULL)
        printf("fieldList is NULL\n");
    else {
        printf("fieldList name is: %s\n", fieldList->name);
        printf("FieldList Type:\n");
        printType(fieldList->type);
        printFieldList(fieldList->tail);
    }
}

// tableItem functions
pItem newItem(int symbolDepth, pFieldList pfield) {
    pItem p = (pItem)malloc(sizeof(TableItem));
    assert(p != NULL);
    p->symbolDepth = symbolDepth;
    p->field = pfield;
    p->nextHash = NULL;
    p->nextSymbol = NULL;
    return p;
}

void deleteItem(pItem item) {
    assert(item != NULL);
    if (item->field != NULL) deleteFieldList(item->field);
    free(item);
}

// Hash functions
pHash newHash() {
    pHash p = (pHash)malloc(sizeof(HashTable));
    assert(p != NULL);
    p->hashArray = (pItem*)malloc(sizeof(pItem) * HASH_TABLE_SIZE);
    assert(p->hashArray != NULL);
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        p->hashArray[i] = NULL;
    }
    return p;
}

void deleteHash(pHash hash) {
    assert(hash != NULL);
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        pItem temp = hash->hashArray[i];
        while (temp) {
            pItem tdelete = temp;
            temp = temp->nextHash;
            deleteItem(tdelete);
        }
        hash->hashArray[i] = NULL;
    }
    free(hash->hashArray);
    hash->hashArray = NULL;
    free(hash);
}

pItem getHashHead(pHash hash, int index) {
    assert(hash != NULL);
    return hash->hashArray[index];
}

void setHashHead(pHash hash, int index, pItem newVal) {
    assert(hash != NULL);
    hash->hashArray[index] = newVal;
}
// Table functions

pTable initTable() {
    pTable table = (pTable)malloc(sizeof(Table));
    assert(table != NULL);
    table->hash = newHash();
    table->stack = newStack();
    table->unNamedStructNum = 0;
    return table;
};

void deleteTable(pTable table) {
    deleteHash(table->hash);
    table->hash = NULL;
    deleteStack(table->stack);
    table->stack = NULL;
    free(table);
};

pItem searchTableItem(pTable table, char* name) {
    unsigned hashCode = getHashCode(name);
    pItem temp = getHashHead(table->hash, hashCode);
    if (temp == NULL) return NULL;
    while (temp) {
        if (!strcmp(temp->field->name, name)) return temp;
        temp = temp->nextHash;
    }
    return NULL;
}

// Return false -> no confliction, true -> has confliction
boolean checkTableItemConflict(pTable table, pItem item) { // 查找当前表项是否与已有的冲突
    pItem temp = searchTableItem(table, item->field->name); // 根据hashCode查找
    if (temp == NULL) return FALSE;
    while (temp) { // 遍历哈希冲突链表
        if (!strcmp(temp->field->name, item->field->name)) { // name相同
            if (temp->field->type->kind == STRUCTURE || 
                item->field->type->kind == STRUCTURE) // 结构体名不允许与任意作用域的其他标识符重复
                return TRUE;
            if (temp->symbolDepth == table->stack->curStackDepth) return TRUE; // 同一作用域，冲突
        }
        temp = temp->nextHash;
    }
    return FALSE;
}

void addTableItem(pTable table, pItem item) {
    assert(table != NULL && item != NULL);
    unsigned hashCode = getHashCode(item->field->name);
    pHash hash = table->hash;
    pStack stack = table->stack;
    // if (getCurDepthStackHead(stack) == NULL)
    //     setCurDepthStackHead(stack, item);
    // else {
    //     item->nextHash = getCurDepthStackHead(stack);
    //     setCurDepthStackHead(stack, item);
    // }
    item->nextSymbol = getCurDepthStackHead(stack);
    setCurDepthStackHead(stack, item);

    item->nextHash = getHashHead(hash, hashCode);
    setHashHead(hash, hashCode, item);
}

void deleteTableItem(pTable table, pItem item) {
    assert(table != NULL && item != NULL);
    unsigned hashCode = getHashCode(item->field->name);
    if (item == getHashHead(table->hash, hashCode))
        setHashHead(table->hash, hashCode, item->nextHash);
    else {
        pItem cur = getHashHead(table->hash, hashCode);
        pItem last = cur;
        while (cur != item) {
            last = cur;
            cur = cur->nextHash;
        }
        last->nextHash = cur->nextHash;
    }
    deleteItem(item);
}

boolean isStructDef(pItem src) {
    if (src == NULL) return FALSE;
    if (src->field->type->kind != STRUCTURE) return FALSE;
    if (src->field->type->u.structure.structName) return FALSE;
    return TRUE;
}

// void addStructLayer(pTable table) { table->enterStructLayer++; }

// void minusStructLayer(pTable table) { table->enterStructLayer--; }

// boolean isInStructLayer(pTable table) { return table->enterStructLayer > 0; }

void clearCurDepthStackList(pTable table) {
    assert(table != NULL);
    pStack stack = table->stack;
    pItem temp = getCurDepthStackHead(stack);
    while (temp) {
        pItem tDelete = temp;
        temp = temp->nextSymbol;
        deleteTableItem(table, tDelete);
    }
    setCurDepthStackHead(stack, NULL);
    minusStackDepth(stack);
}

// for Debug
void printTable(pTable table) {
    printf("----------------hash_table----------------\n");
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        pItem item = getHashHead(table->hash, i);
        if (item) {
            printf("[%d]", i);
            while (item) {
                printf(" -> name: %s depth: %d\n", item->field->name,
                       item->symbolDepth);
                printf("========FiledList========\n");
                printFieldList(item->field);
                printf("===========End===========\n");
                item = item->nextHash;
            }
            printf("\n");
        }
    }
    printf("-------------------end--------------------\n");
}

// Stack functions
pStack newStack() {
    pStack p = (pStack)malloc(sizeof(Stack));
    assert(p != NULL);
    p->stackArray = (pItem*)malloc(sizeof(pItem) * HASH_TABLE_SIZE);
    assert(p->stackArray != NULL);
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        p->stackArray[i] = NULL;
    }
    p->curStackDepth = 0;
    return p;
}

void deleteStack(pStack stack) {
    assert(stack != NULL);
    free(stack->stackArray);
    stack->stackArray = NULL;
    stack->curStackDepth = 0;
    free(stack);
}

void addStackDepth(pStack stack) {
    assert(stack != NULL);
    stack->curStackDepth++;
}

void minusStackDepth(pStack stack) {
    assert(stack != NULL);
    stack->curStackDepth--;
}

pItem getCurDepthStackHead(pStack stack) {
    assert(stack != NULL);
    return stack->stackArray[stack->curStackDepth];
    // return p == NULL ? NULL : p->stackArray[p->curStackDepth];
}

void setCurDepthStackHead(pStack stack, pItem newVal) {
    assert(stack != NULL);
    stack->stackArray[stack->curStackDepth] = newVal;
}

// Global function
void traverseTree(pNode node) {
    if (node == NULL) return;

    if (!strcmp(node->name, "ExtDef")) ExtDef(node);

    traverseTree(node->child);
    traverseTree(node->next);
}

// Generate symbol table functions
void ExtDef(pNode node) {
    assert(node != NULL);
    // ExtDef -> Specifier ExtDecList SEMI
    //         | Specifier SEMI
    //         | Specifier FunDec CompSt
    pType specifierType = Specifier(node->child); //获取Specifier的类型 TYPE/StructSpecifier，传入子过程的处理函数
    char* secondName = node->child->next->name;   //根据二儿子的名称调用不同的子过程

    // printType(specifierType);
    // ExtDef -> Specifier ExtDecList SEMI
    if (!strcmp(secondName, "ExtDecList")) {
        ExtDecList(node->child->next, specifierType);
    }
    // ExtDef -> Specifier FunDec CompSt
    else if (!strcmp(secondName, "FunDec")) {
        // TODO: process third situation
        FunDec(node->child->next, specifierType);
        CompSt(node->child->next->next, specifierType);
    }
    if (specifierType) deleteType(specifierType);
    // printTable(table);
    // Specifier SEMI
    // this situation has no meaning
    // or is struct define(have been processe in Specifier())
}

void ExtDecList(pNode node, pType specifier) {
    assert(node != NULL);
    // ExtDecList -> VarDec
    //             | VarDec COMMA ExtDecList
    pNode temp = node;
    while (temp) {
        pItem item = VarDec(temp->child, specifier);
        if (checkTableItemConflict(table, item)) {
            char msg[100] = {0};
            sprintf(msg, "Redefined variable \"%s\".", item->field->name);
            pError(REDEF_VAR, temp->lineNo, msg);
            deleteItem(item);
        } else {
            addTableItem(table, item);
        }
        if (temp->child->next) {
            temp = temp->next->next->child; //递归找varDec
        } else {
            break;
        }
    }
}

pType Specifier(pNode node) {
    assert(node != NULL);
    // Specifier -> TYPE
    //            | StructSpecifier

    pNode t = node->child;
    // Specifier -> TYPE
    if (!strcmp(t->name, "TYPE")) {
        if (!strcmp(t->val, "float")) {
            return newType(BASIC, FLOAT_TYPE);
        } else {
            return newType(BASIC, INT_TYPE);
        }
    }
    // Specifier -> StructSpecifier
    else {
        return StructSpecifier(t);
    }
}

pType StructSpecifier(pNode node) {
    assert(node != NULL);
    // StructSpecifier -> STRUCT OptTag LC DefList RC
    //                  | STRUCT Tag

    // OptTag -> ID | e
    // Tag -> ID
    pType returnType = NULL;
    pNode t = node->child->next;
    // StructSpecifier->STRUCT OptTag LC DefList RC
    // printTreeInfo(t, 0);
    if (strcmp(t->name, "Tag")) { //STRUCT OptTag LC DefList RC
        pItem structItem =
            newItem(table->stack->curStackDepth,
                    newFieldList("", newType(STRUCTURE, NULL, NULL)));//新建结构体符号表项，name和field暂置为空
        //设置结构体名称
        // named struct
        if (!strcmp(t->name, "OptTag")) {
            setFieldListName(structItem->field, t->child->val); //用结构体ID给structItem->field->name赋值
            t = t->next; //t: LC
        }
        // unnamed struct
        else {
            table->unNamedStructNum++;
            char structName[20] = {0};
            sprintf(structName, "%d", table->unNamedStructNum);
            // printf("unNamed struct's name is %s.\n", structName);
            setFieldListName(structItem->field, structName); //给结构体一个新的编号
        }
        //收集结构体字段，添加到structItem中
        if (!strcmp(t->next->name, "DefList")) {
            DefList(t->next, structItem);
        }
        //检查重名并添加符号表项
        if (checkTableItemConflict(table, structItem)) { //检查是否有重名
            char msg[100] = {0};
            sprintf(msg, "Duplicated name \"%s\".", structItem->field->name);
            pError(DUPLICATED_NAME, node->lineNo, msg);
            deleteItem(structItem);
        } else { //没有重名
            returnType = newType( // 构造并返回结构体类型 pType
                STRUCTURE, newString(structItem->field->name),
                copyFieldList(structItem->field->type->u.structure.field));

            // printf("\nnew Type:\n");
            // printType(returnType);
            // printf("\n");

            if (!strcmp(node->child->next->name, "OptTag")) {
                addTableItem(table, structItem); //有名字的结构体加入符号表
            }
            // OptTag -> e
            else {
                deleteItem(structItem);
            }
        }

    }

    // StructSpecifier->STRUCT Tag
    else { 
        pItem structItem = searchTableItem(table, t->child->val);
        if (structItem == NULL || !isStructDef(structItem)) {
            char msg[100] = {0};
            sprintf(msg, "Undefined structure \"%s\".", t->child->val);
            pError(UNDEF_STRUCT, node->lineNo, msg);
        } else
            returnType = newType(
                STRUCTURE, newString(structItem->field->name),
                copyFieldList(structItem->field->type->u.structure.field));
    }
    // printType(returnType);
    return returnType;
}

pItem VarDec(pNode node, pType specifier) { //返回ID的符号表项
    assert(node != NULL);
    // VarDec -> ID
    //         | VarDec LB INT RB
    pNode id = node;
    // get ID
    while (id->child) id = id->child;
    pItem p = newItem(table->stack->curStackDepth, newFieldList(id->val, NULL)); //新建符号表项
    // return newItem(table->stack->curStackDepth,
    //                newFieldList(id->val, generateVarDecType(node,
    //                specifier)));

    // VarDec -> ID
    // printTreeInfo(node, 0);
    if (!strcmp(node->child->name, "ID")) { //普通标识符
        // printf("copy type tp %s.\n", node->child->val);
        p->field->type = copyType(specifier); //把specifier的类型赋值给符号表项的域类型字段
    }
    // VarDec -> VarDec LB INT RB
    else { //数组
        pNode varDec = node->child;
        pType temp = specifier;
        // printf("VarDec -> VarDec LB INT RB.\n");
        // 递归填充p的域类型（以int a[5][10]为例）：
        //ARRAY (size=5)
        //  └── ARRAY (size=10)
        //        └── BASIC (int)
        while (varDec->next) { //最后一个是varDec->ID
            // printTreeInfo(varDec, 0);
            // printf("number: %s\n", varDec->next->next->val);
            // printf("temp type: %d\n", temp->kind);
            p->field->type =
                newType(ARRAY, copyType(temp), atoi(varDec->next->next->val));
            // printf("newType. newType: elem type: %d, elem size: %d.\n",
            //        p->field->type->u.array.elem->kind,
            //        p->field->type->u.array.size);
            temp = p->field->type;
            varDec = varDec->child;
        }
    }
    // printf("-------test VarDec ------\n");
    // printType(specifier);
    // printFieldList(p->field);
    // printf("-------test End ------\n");
    return p;
}

// pType generateVarDecType(pNode node, pType type) {
//     // VarDec -> ID
//     if (!strcmp(node->child->name, "ID")) return copyType(type);
//     // VarDec -> VarDec LB INT RB
//     else
//         return newType(ARRAY, atoi(node->child->next->next->val),
//                        generateVarDecType(node, type));
// }

void FunDec(pNode node, pType returnType) {
    assert(node != NULL);
    // FunDec -> ID LP VarList RP
    //         | ID LP RP
    pItem p =
        newItem(table->stack->curStackDepth,
                newFieldList(node->child->val,
                             newType(FUNCTION, 0, NULL, copyType(returnType))));
                             //传参格式：FUNCTION，参数个数，参数列表（字段链表），返回类型
                             //参数个数先设置为0个，后续在VarList过程中补充

    // FunDec -> ID LP VarList RP
    if (!strcmp(node->child->next->next->name, "VarList")) {
        VarList(node->child->next->next, p);
    }

    // FunDec -> ID LP RP don't need process

    // check redefine
    if (checkTableItemConflict(table, p)) {
        char msg[100] = {0};
        sprintf(msg, "Redefined function \"%s\".", p->field->name);
        pError(REDEF_FUNC, node->lineNo, msg);
        deleteItem(p);
        p = NULL;
    } else {
        addTableItem(table, p);
    }
}

void VarList(pNode node, pItem func) {
    assert(node != NULL);
    // VarList -> ParamDec COMMA VarList
    //          | ParamDec
    addStackDepth(table->stack); //进入参数作用域
    int argc = 0;
    pNode temp = node->child; //temp指向VarList的第一个ParamDec
    pFieldList cur = NULL;    //参数链表指针

    // VarList -> ParamDec
    pFieldList paramDec = ParamDec(temp);//返回当前ParamDec的参数链表
    func->field->type->u.function.argv = copyFieldList(paramDec);//将当前ParamDec对应的参数链表拷贝到函数的参数链表
    cur = func->field->type->u.function.argv;
    argc++;

    // VarList -> ParamDec COMMA VarList
    while (temp->next) {//直到VarList->ParamDec
        temp = temp->next->next->child;//下一个 ParamDec
        paramDec = ParamDec(temp);//参数链表
        if (paramDec) {
            cur->tail = copyFieldList(paramDec);//拷贝
            cur = cur->tail;
            argc++;//参数个数加一个
        }
    }

    func->field->type->u.function.argc = argc;

    minusStackDepth(table->stack); //离开参数作用域
}

pFieldList ParamDec(pNode node) {
    assert(node != NULL);
    // ParamDec -> Specifier VarDec
    pType specifierType = Specifier(node->child);
    pItem p = VarDec(node->child->next, specifierType);
    if (specifierType) deleteType(specifierType);
    if (checkTableItemConflict(table, p)) {
        char msg[100] = {0};
        sprintf(msg, "Redefined variable \"%s\".", p->field->name);
        pError(REDEF_VAR, node->lineNo, msg);
        deleteItem(p);
        return NULL;
    } else {
        addTableItem(table, p);
        return p->field;
    }
}

void CompSt(pNode node, pType returnType) { //returnType是调用时传入的函数返回值类型
    assert(node != NULL);
    // CompSt -> LC DefList StmtList RC
    // printTreeInfo(node, 0);
    addStackDepth(table->stack); //进入函数作用域
    pNode temp = node->child->next;
    if (!strcmp(temp->name, "DefList")) {
        DefList(temp, NULL);
        temp = temp->next;
    }
    if (!strcmp(temp->name, "StmtList")) {
        StmtList(temp, returnType);
    }

    clearCurDepthStackList(table);//销毁局部变量并退出当前函数作用域，已经包含minusStackDepth
}

void StmtList(pNode node, pType returnType) {
    // assert(node != NULL);
    // StmtList -> Stmt StmtList
    //           | e
    // printTreeInfo(node, 0);
    while (node) {
        Stmt(node->child, returnType);
        node = node->child->next;
    }
}

void Stmt(pNode node, pType returnType) {
    assert(node != NULL);
    // Stmt -> Exp SEMI
    //       | CompSt
    //       | RETURN Exp SEMI
    //       | IF LP Exp RP Stmt
    //       | IF LP Exp RP Stmt ELSE Stmt
    //       | WHILE LP Exp RP Stmt
    // printTreeInfo(node, 0);

    pType expType = NULL;
    // Stmt -> Exp SEMI
    if (!strcmp(node->child->name, "Exp")) expType = Exp(node->child);

    // Stmt -> CompSt
    else if (!strcmp(node->child->name, "CompSt"))
        CompSt(node->child, returnType);

    // Stmt -> RETURN Exp SEMI
    else if (!strcmp(node->child->name, "RETURN")) {
        expType = Exp(node->child->next);

        // check return type
        if (!checkType(returnType, expType))
            pError(TYPE_MISMATCH_RETURN, node->lineNo,
                   "Type mismatched for return."); //返回值类型不匹配
    }

    // Stmt -> IF LP Exp RP Stmt
    else if (!strcmp(node->child->name, "IF")) {
        //addStackDepth(table->stack);
        pNode stmt = node->child->next->next->next->next;
        expType = Exp(node->child->next->next);
        Stmt(stmt, returnType);
        // Stmt -> IF LP Exp RP Stmt ELSE Stmt
        if (stmt->next != NULL) Stmt(stmt->next->next, returnType);
        //minusStackDepth(table->stack);
    }

    // Stmt -> WHILE LP Exp RP Stmt
    else if (!strcmp(node->child->name, "WHILE")) {
        //addStackDepth(table->stack);
        expType = Exp(node->child->next->next);
        Stmt(node->child->next->next->next->next, returnType);
        //minusStackDepth(table->stack);
    }

    if (expType) deleteType(expType);
}

void DefList(pNode node, pItem structInfo) { //structInfo用于StructSpecifier->STRUCT OptTag LC DefList RC 
    // assert(node != NULL);
    // DefList -> Def DefList
    //          | e
    while (node) {
        Def(node->child, structInfo);
        node = node->child->next;
    }
}

void Def(pNode node, pItem structInfo) {
    assert(node != NULL);
    // Def -> Specifier DecList SEMI
    pType dectype = Specifier(node->child);
    DecList(node->child->next, dectype, structInfo);
    if (dectype) deleteType(dectype);
}

void DecList(pNode node, pType specifier, pItem structInfo) {
    assert(node != NULL);
    // DecList -> Dec
    //          | Dec COMMA DecList
    pNode temp = node;
    while (temp) {//递归找Dec
        Dec(temp->child, specifier, structInfo);
        if (temp->child->next)
            temp = temp->child->next->next;
        else
            break;
    }
}
void Dec(pNode node, pType specifier, pItem structInfo) { //检查变量声明
    assert(node != NULL);
    // Dec -> VarDec
    //      | VarDec ASSIGNOP Exp

    if (node->child->next == NULL) { // Dec->VarDec
        if (structInfo != NULL) {    // 结构体内的变量声明
            // 添加 null 检查，避免 field 为 NULL 时崩溃
            if (structInfo->field == NULL || structInfo->field->type == NULL || 
                structInfo->field->type->kind != STRUCTURE) { // structInfo不合法
                pError(REDEF_FEILD, node->lineNo, "Invalid structInfo for Dec.");
                return;
            }

            pItem decitem = VarDec(node->child, specifier);
            if (decitem == NULL || decitem->field == NULL) { //检查空指针
                pError(REDEF_FEILD, node->lineNo, "Invalid declaration in struct.");
                deleteItem(decitem);
                return;
            }

            pFieldList payload = decitem->field; //新添加的VarDec对应的字段
            pFieldList structField = structInfo->field->type->u.structure.field; //已有的字段
            pFieldList last = NULL;

            while (structField != NULL) { //遍历已有字段，检查是否重复定义
                if (structField->name && payload->name &&
                    strcmp(payload->name, structField->name) == 0) { // 结构体域定义重复
                    char msg[100] = {0};
                    sprintf(msg, "Redefined field \"%s\".", payload->name);
                    pError(REDEF_FEILD, node->lineNo, msg);
                    deleteItem(decitem);
                    return;
                }
                last = structField;
                structField = structField->tail;
            }

            if (last == NULL) { //还没有字段，挂在结构体的第一个字段
                structInfo->field->type->u.structure.field = copyFieldList(payload);
            } else { //已经有字段，挂在最后一个字段后面
                last->tail = copyFieldList(payload);
            }
            deleteItem(decitem);
        } else { // 普通变量声明
            pItem decitem = VarDec(node->child, specifier);
            if (decitem == NULL || decitem->field == NULL) {
                pError(REDEF_VAR, node->lineNo, "Invalid variable declaration.");
                deleteItem(decitem);
                return;
            }
            if (checkTableItemConflict(table, decitem)) {
                char msg[100] = {0};
                sprintf(msg, "Redefined variable \"%s\".", decitem->field->name);
                pError(REDEF_VAR, node->lineNo, msg);
                deleteItem(decitem);
            } else {
                addTableItem(table, decitem);
            }
        }
    } else { // Dec->VarDec ASSIGNOP Exp
        if (structInfo != NULL) {
            pError(REDEF_FEILD, node->lineNo,
                   "Illegal initialize variable in struct.");
        } else {
            pItem decitem = VarDec(node->child, specifier);
            pType exptype = Exp(node->child->next->next);

            if (decitem == NULL || decitem->field == NULL) {
                pError(REDEF_VAR, node->lineNo, "Invalid variable declaration.");
                deleteItem(decitem);
                if (exptype) deleteType(exptype);
                return;
            }

            if (checkTableItemConflict(table, decitem)) {
                char msg[100] = {0};
                sprintf(msg, "Redefined variable \"%s\".", decitem->field->name);
                pError(REDEF_VAR, node->lineNo, msg);
                deleteItem(decitem);
                if (exptype) deleteType(exptype);
                return;
            }

            if (!checkType(decitem->field->type, exptype)) {
                pError(TYPE_MISMATCH_ASSIGN, node->lineNo,
                       "Type mismatched for assignment."); //类型不匹配
                deleteItem(decitem);
                if (exptype) deleteType(exptype);
                return;
            }

            if (decitem->field->type && decitem->field->type->kind == ARRAY) { //不许对数组赋值
                pError(TYPE_MISMATCH_ASSIGN, node->lineNo,
                       "Illegal initialize variable.");
                deleteItem(decitem);
                if (exptype) deleteType(exptype);
                return;
            }

            addTableItem(table, decitem); //添加到符号表
            if (exptype) deleteType(exptype);
        }
    }
}

pType Exp(pNode node) { //返回表达式的类型
    assert(node != NULL);
    // Exp -> Exp ASSIGNOP Exp
    //      | Exp AND Exp
    //      | Exp OR Exp
    //      | Exp RELOP Exp
    //      | Exp PLUS Exp
    //      | Exp MINUS Exp
    //      | Exp STAR Exp
    //      | Exp DIV Exp
    //      | LP Exp RP
    //      | MINUS Exp
    //      | NOT Exp
    //      | ID LP Args RP
    //      | ID LP RP
    //      | Exp LB Exp RB
    //      | Exp DOT ID
    //      | ID
    //      | INT
    //      | FLOAT
    pNode t = node->child;
    // exp will only check if the cal is right
    //  printTable(table);
    //二值运算
    if (!strcmp(t->name, "Exp")) {
        // 基本数学运算符
        if (strcmp(t->next->name, "LB") && strcmp(t->next->name, "DOT")) { //不是数组，也不是结构体访问
            pType p1 = Exp(t);
            pType p2 = Exp(t->next->next);
            pType returnType = NULL;

            // Exp -> Exp ASSIGNOP Exp
            if (!strcmp(t->next->name, "ASSIGNOP")) { //赋值运算符
                //检查左值
                pNode tchild = t->child;

                if( (!strcmp(tchild->name, "ID") &&
                           (!tchild->next || strcmp(tchild->next->name, "LP")))|| //不能是函数调用
                           !strcmp(tchild->next->name, "LB") ||
                           !strcmp(tchild->next->name, "DOT")) { //允许的左值
                    if (!checkType(p1, p2)) { // 检查左右值类型是否匹配
                        //报错，类型不匹配
                        pError(TYPE_MISMATCH_ASSIGN, t->lineNo,
                               "Type mismatched for assignment.");
                    } else
                        returnType = copyType(p1);
                } else {
                    //报错，左值
                    pError(LEFT_VAR_ASSIGN, t->lineNo,
                           "The left-hand side of an assignment must be "
                           "a variable.");
                }

            }
            // Exp -> Exp AND Exp
            //      | Exp OR Exp
            //      | Exp RELOP Exp
            //      | Exp PLUS Exp
            //      | Exp MINUS Exp
            //      | Exp STAR Exp
            //      | Exp DIV Exp
            else {
                if (p1 && p2 && (p1->kind == ARRAY || p2->kind == ARRAY)) {
                    //报错，数组，结构体运算
                    pError(TYPE_MISMATCH_OP, t->lineNo,
                           "Type mismatched for operands."); //数组和结构体不能作为操作数
                } else if (!checkType(p1, p2)) {
                    //报错，类型不匹配
                    pError(TYPE_MISMATCH_OP, t->lineNo,
                           "Type mismatched for operands.");
                } else {
                    if (p1 && p2) {
                        returnType = copyType(p1);//正确则返回任意操作数类型
                    }
                }
            }

            if (p1) deleteType(p1);
            if (p2) deleteType(p2);
            return returnType;
        }
        // 数组和结构体访问
        else {
            // Exp -> Exp LB Exp RB
            if (!strcmp(t->next->name, "LB")) {
                //数组
                pType p1 = Exp(t);//数组变量
                pType p2 = Exp(t->next->next);//数组下标
                pType returnType = NULL;

                if (!p1) {
                    // 第一个exp为null，上层报错，这里不用再管
                } else if (p1 && p1->kind != ARRAY) {
                    //报错，对非数组使用[]运算符
                    char msg[100] = {0};
                    sprintf(msg, "\"%s\" is not an array.", t->child->val);
                    pError(NOT_A_ARRAY, t->lineNo, msg);
                } else if (!p2 || p2->kind != BASIC ||
                           p2->u.basic != INT_TYPE) {
                    //报错，下标不是整数
                    char msg[100] = {0};
                    sprintf(msg, "\"%s\" is not an integer.",
                            t->next->next->child->val);
                    pError(NOT_A_INT, t->lineNo, msg);
                } else {
                    returnType = copyType(p1->u.array.elem); //正确，返回数组元素类型
                }
                if (p1) deleteType(p1);
                if (p2) deleteType(p2);
                return returnType;
            }
            // Exp -> Exp DOT ID
            else {
                pType p1 = Exp(t);
                pType returnType = NULL;
                if (!p1 || p1->kind != STRUCTURE ||
                    !p1->u.structure.structName) {
                    //报错，对非结构体使用.运算符
                    pError(ILLEGAL_USE_DOT, t->lineNo, "Illegal use of \".\".");
                    if (p1) deleteType(p1);
                } else {
                    pNode ref_id = t->next->next;
                    pFieldList structfield = p1->u.structure.field;
                    while (structfield != NULL) {
                        if (!strcmp(structfield->name, ref_id->val)) {
                            break;
                        }
                        structfield = structfield->tail;
                    }
                    if (structfield == NULL) {
                        //报错，没有可以匹配的域名
                        printf("Error type %d at Line %d: Non-existent field \"%s\".\n", 14, t->lineNo, ref_id->val);
                    } else {
                        returnType = copyType(structfield->type);//正确，返回字段类型
                    }
                }
                //if (p1) deleteType(p1);
                return returnType;
            }
        }
    }
    //单目运算符
    // Exp -> MINUS Exp
    //      | NOT Exp
    else if (!strcmp(t->name, "MINUS") || !strcmp(t->name, "NOT")) {
        pType p1 = Exp(t->next);
        pType returnType = NULL;
        if (!p1 || p1->kind != BASIC) {
            //报错，数组，结构体运算
            printf("Error type %d at Line %d: %s.\n", 7, t->lineNo,
                   "TYPE_MISMATCH_OP");
        } else {
            returnType = copyType(p1);
        }
        if (p1) deleteType(p1);
        return returnType;
    } else if (!strcmp(t->name, "LP")) { //Exp -> LP Exp RP
        return Exp(t->next);
    }
    // Exp -> ID LP Args RP
    //		| ID LP RP
    else if (!strcmp(t->name, "ID") && t->next) { //函数调用
        pItem funcInfo = searchTableItem(table, t->val); //根据函数名查找符号表

        // function not find
        if (funcInfo == NULL) { //函数未定义
            char msg[100] = {0};
            sprintf(msg, "Undefined function \"%s\".", t->val);
            pError(UNDEF_FUNC, node->lineNo, msg);
            return NULL;
        } else if (funcInfo->field->type->kind != FUNCTION) { //标识符不是函数
            char msg[100] = {0};
            sprintf(msg, "\"%s\" is not a function.", t->val);
            pError(NOT_A_FUNC, node->lineNo, msg);
            return NULL;
        }
        // Exp -> ID LP Args RP
        else if (!strcmp(t->next->next->name, "Args")) {
            Args(t->next->next, funcInfo);//参数检查
            return copyType(funcInfo->field->type->u.function.returnType);//返回函数返回值类型
        }
        // Exp -> ID LP RP
        else {
            if (funcInfo->field->type->u.function.argc != 0) {
                char msg[100] = {0}; //少参数
                sprintf(msg,
                        "too few arguments to function \"%s\", except %d args.",
                        funcInfo->field->name,
                        funcInfo->field->type->u.function.argc);
                pError(FUNC_AGRC_MISMATCH, node->lineNo, msg);
            }
            return copyType(funcInfo->field->type->u.function.returnType);
        }
    }
    // Exp -> ID
    else if (!strcmp(t->name, "ID")) {
        pItem tp = searchTableItem(table, t->val); //根据ID查找符号表
        if (tp == NULL || isStructDef(tp)) {
            char msg[100] = {0};
            sprintf(msg, "Undefined variable \"%s\".", t->val); //变量未定义
            pError(UNDEF_VAR, t->lineNo, msg);
            return NULL;
        } else {
            // good
            return copyType(tp->field->type);//返回ID的类型
        }
    } else {//常量
        // Exp -> FLOAT
        if (!strcmp(t->name, "FLOAT")) {
            return newType(BASIC, FLOAT_TYPE);
        }
        // Exp -> INT
        else {
            return newType(BASIC, INT_TYPE);
        }
    }
}

void Args(pNode node, pItem funcInfo) {//函数参数检查
    assert(node != NULL);
    // Args -> Exp COMMA Args
    //       | Exp
    // printTreeInfo(node, 0);
    pNode temp = node;
    pFieldList arg = funcInfo->field->type->u.function.argv;//由符号表获知的函数形参列表
    // printf("-----function atgs-------\n");
    // printFieldList(arg);
    // printf("---------end-------------\n");
    while (temp) {//遍历实参和形参，一一检查
        if (arg == NULL) {//形参列表已经遍历完，但还有实参
            char msg[100] = {0};
            sprintf(
                msg, "too many arguments to function \"%s\", except %d args.",
                funcInfo->field->name, funcInfo->field->type->u.function.argc);
            pError(FUNC_AGRC_MISMATCH, node->lineNo, msg);
            break;
        }
        pType realType = Exp(temp->child);//获取实参类型
        // printf("=======arg type=========\n");
        // printType(realType);
        // printf("===========end==========\n");
        if (!checkType(realType, arg->type)) {//实参类型和形参类型不匹配
            char msg[100] = {0};
            sprintf(msg, "Function \"%s\" is not applicable for arguments.",
                    funcInfo->field->name);
            pError(FUNC_AGRC_MISMATCH, node->lineNo, msg);
            if (realType) deleteType(realType);
            return;
        }
        if (realType) deleteType(realType);

        arg = arg->tail;//下一个形参
        if (temp->child->next) {
            temp = temp->child->next->next;//下一个实参
        } else {
            break;
        }
    }
    if (arg != NULL) { //形参列表还有剩余
        char msg[100] = {0};
        sprintf(msg, "too few arguments to function \"%s\", except %d args.",
                funcInfo->field->name, funcInfo->field->type->u.function.argc);
        pError(FUNC_AGRC_MISMATCH, node->lineNo, msg);
    }
}
