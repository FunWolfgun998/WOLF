//
// Created by funwolfgun on 6/8/26.
//
#include "../../include/Utils/ASTPrinter.h"

// -- Expressions (from ExprVariant) --
void ASTPrinter::operator()(const std::unique_ptr<LiteralExpr>& node) {
    // Print the literal value exactly as it was written in the code
    out << "LiteralExpr(" << node->value.lexeme << ")\n";
}

void ASTPrinter::operator()(const std::unique_ptr<VariableExpr>& node) {
    // Print the variable name
    out << "VariableExpr(" << node->name.lexeme << ")\n";
}

void ASTPrinter::operator()(const std::unique_ptr<BinaryExpr>& node) {
    out << "BinaryExpr(Op: " << node->op.lexeme << ")\n";
    // Visit left branch (not the last child)
    printBrach(node->left.as, false, "Left");
    // Visit right branch (as the last child)
    printBrach(node->right.as, true, "Right");
}

void ASTPrinter::operator()(const std::unique_ptr<UnaryExpr>& node) {
    // Unary only has one child, so it's always the last
    if (node->isPostfix) {
        out << "PostfixUnaryExpr(Op: " << node->op.lexeme << ")\n";
        printBrach(node->operand.as, true, "Left");
    } else {
        out << "PrefixUnaryExpr(Op: " << node->op.lexeme << ")\n";
        printBrach(node->operand.as, true, "Right");
    }
}
void ASTPrinter::operator()(const std::unique_ptr<GroupingExpr>& node) {
    out << "GroupingExpr\n";
    printBrach(node->expression.as, true, "Expression");
}

void ASTPrinter::operator()(const std::unique_ptr<CallExpr>& node) {
    out << "CallExpr\n";

    // The callee is only the 'last' branch if there are no arguments
    bool noArgs = node->arguments.empty();
    printBrach(node->callee.as, noArgs, "Callee");

    // Visit all arguments
    for (size_t i = 0; i < node->arguments.size(); ++i) {
        bool isLast = (i == node->arguments.size() - 1);
        printBrach(node->arguments[i].as, isLast, "Arg[" + std::to_string(i) + "]");
    }
}

void ASTPrinter::operator()(const std::unique_ptr<MemberAccessExpr>& node) {
    out << "MemberAccessExpr(Field: " << node->member.lexeme << ")\n";
    printBrach(node->accessed.as, true, "Object");
}

void ASTPrinter::operator()(const std::unique_ptr<ArrayLiteralExpr>& node) {
    out << "ArrayLiteralExpr\n";

    // Empty array
    if (node->elements.empty()) {
        out << currentPrefix << "└── (Empty Array)\n";
        return;
    }

    // Loop with all nodes
    for (size_t i = 0; i < node->elements.size(); ++i) {
        bool isLast = (i == node->elements.size() - 1);
        std::string label = "Element[" + std::to_string(i) + "]";
        printBrach(node->elements[i].as, isLast, label);
    }
}

void ASTPrinter::operator()(const std::unique_ptr<ArrayAccessExpr>& node) {
    out << "ArrayAccessExpr\n";
    printBrach(node->array.as, false, "Array");
    printBrach(node->index.as, true, "Index");
}

void ASTPrinter::operator()(const std::unique_ptr<TernaryExpr>& node) {
    out << "TernaryExpr\n";
    printBrach(node->condition.as, false, "Condition");
    printBrach(node->trueBranch.as, false, "TrueBranch");
    printBrach(node->falseBranch.as, true, "FalseBranch");
}

// -- Statements (from StmtVariant) --

void ASTPrinter::operator()(const std::unique_ptr<ExpressionStmt>& node) {
    out << "ExpressionStmt\n";
    printBrach(node->expression.as, true, "Expression");
}

void ASTPrinter::operator()(const std::unique_ptr<VarDeclStmt>& node) {
    out << "VarDeclStmt(Type: " << node->type.lexeme << ", Name: " << node->name.lexeme << ")\n";

    // Null safety check: variables might not have an initializer (e.g. `int x;`)
    if (node->initializer != nullptr) {
        printBrach(node->initializer->as, true, "Initializer");
    }
}

void ASTPrinter::operator()(const std::unique_ptr<BlockStmt>& node) {
    out << "BlockStmt\n";
    for (size_t i = 0; i < node->statements.size(); ++i) {
        bool isLast = (i == node->statements.size() - 1);
        printBrach(node->statements[i].as, isLast);
    }
}

void ASTPrinter::operator()(const std::unique_ptr<IfStmt>& node) {
    out << "IfStmt\n";

    bool hasElifs = !node->elifBranches.empty();
    bool hasElse = node->elseBranch != nullptr;

    // 1. Condizione
    printBrach(node->condition.as, false, "Condition");

    // 2. Then Branch - Ora è un semplice Stmt, usiamo printBrach!
    // È l'ultimo ramo solo se non ci sono elif o else
    printBrach(node->thenBranch.as, !(hasElifs || hasElse), "Then");

    // 3. ELIF Branches - Dobbiamo ciclare il vettore, ma il corpo è uniforme
    for(size_t j = 0; j < node->elifBranches.size(); ++j) {
        bool isLastElif = (j == node->elifBranches.size() - 1) && !hasElse;

        out << currentPrefix << (isLastElif ? "└── " : "├── ") << "ElifBranch[" << j << "]\n";

        // Prepariamo il prefisso per scendere nei dettagli dell'elif
        std::string oldPrefix = currentPrefix;
        currentPrefix += (isLastElif ? "    " : "│   ");

        printBrach(node->elifBranches[j].condition.as, false, "Condition");
        printBrach(node->elifBranches[j].block.as, true, "Body");

        currentPrefix = oldPrefix;
    }

    // 4. ELSE Branch
    if (hasElse) {
        printBrach(node->elseBranch->as, true, "Else");
    }
}

void ASTPrinter::operator()(const std::unique_ptr<WhileStmt>& node) {
    out << "WhileStmt\n";
    printBrach(node->condition.as, false, "Condition");

    // Semplificato: il corpo è uno Stmt (che probabilmente contiene un BlockStmt)
    printBrach(node->body.as, true, "Body");
}

void ASTPrinter::operator()(const std::unique_ptr<ReturnStmt>& node) {
    out << "ReturnStmt\n";
    if (node->value != nullptr) {
        printBrach(node->value->as, true, "Value");
    } else {
        out << currentPrefix << "└── (Void Return)\n";
    }
}

void ASTPrinter::operator()(const std::unique_ptr<ForStmt>& node) {
    out << "ForStmt(Iterator: " << node->iteratorVar.lexeme << ")\n";
    printBrach(node->startRange.as, false, "Start");
    printBrach(node->endRange.as, false, "End");

    // Semplificato: visita uniforme del corpo
    printBrach(node->body.as, true, "Body");
}

void ASTPrinter::operator()(const std::unique_ptr<FunctionDeclStmt>& node) {
    out << "FunctionDeclStmt(ReturnType: " << node->returnType.lexeme
        << ", Name: " << node->name.lexeme << ")\n";

    // I parametri rimangono un vettore di struct semplici, li stampiamo qui
    out << currentPrefix << "├── Parameters\n";
    std::string pPrefix = currentPrefix + "│   ";
    if (node->parameters.empty()) {
        out << pPrefix << "└── (None)\n";
    } else {
        for (size_t i = 0; i < node->parameters.size(); ++i) {
            bool isLast = (i == node->parameters.size() - 1);
            out << pPrefix << (isLast ? "└── " : "├── ")
                << node->parameters[i].type.lexeme << " " << node->parameters[i].name.lexeme << "\n";
        }
    }

    // Semplificato: visita uniforme del corpo
    printBrach(node->body.as, true, "Body");
}

void ASTPrinter::operator()(const std::unique_ptr<StructDeclStmt>& node) {
    out << "StructDeclStmt(Name: " << node->name.lexeme << ")\n";

    // Dato che fields è ora std::vector<Stmt>, usiamo printBrach per ognuno!
    if (node->fields.empty()) {
        out << currentPrefix << "└── (Empty)\n";
    } else {
        for (size_t i = 0; i < node->fields.size(); ++i) {
            bool isLast = (i == node->fields.size() - 1);
            printBrach(node->fields[i].as, isLast, "Field[" + std::to_string(i) + "]");
        }
    }
}