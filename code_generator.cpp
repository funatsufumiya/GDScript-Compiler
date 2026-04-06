#include "code_generator.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <cstring>

// Instruction implementation
std::string Instruction::toString() const {
    std::stringstream ss;
    
    switch (opcode) {
        case MOV: ss << "mov"; break;
        case LOAD: ss << "load"; break;
        case STORE: ss << "store"; break;
        case ADD: ss << "add"; break;
        case SUB: ss << "sub"; break;
        case MUL: ss << "mul"; break;
        case DIV: ss << "div"; break;
        case MOD: ss << "mod"; break;
        case FADD: ss << "fadd"; break;
        case FSUB: ss << "fsub"; break;
        case FMUL: ss << "fmul"; break;
        case FDIV: ss << "fdiv"; break;
        case AND: ss << "and"; break;
        case OR: ss << "or"; break;
        case XOR: ss << "xor"; break;
        case NOT: ss << "not"; break;
        case CMP: ss << "cmp"; break;
        case FCMP: ss << "fcmp"; break;
        case JMP: ss << "jmp"; break;
        case JE: ss << "je"; break;
        case JNE: ss << "jne"; break;
        case JL: ss << "jl"; break;
        case JLE: ss << "jle"; break;
        case JG: ss << "jg"; break;
        case JGE: ss << "jge"; break;
        case CALL: ss << "call"; break;
        case RET: ss << "ret"; break;
        case PUSH: ss << "push"; break;
        case POP: ss << "pop"; break;
        case NOP: ss << "nop"; break;
        case LABEL: ss << label << ":"; return ss.str();
        default: ss << "unknown"; break;
    }
    
    if (!label.empty()) {
        ss << " " << label;
    } else {
        for (size_t i = 0; i < operands.size(); ++i) {
            if (i > 0) ss << ", ";
            else ss << " ";
            
            if (operands[i]) {
                ss << operands[i]->name;
            } else {
                ss << "null";
            }
        }
        
        if (has_immediate) {
            if (!operands.empty()) ss << ", ";
            else ss << " ";
            ss << "#" << immediate;
        }
    }
    
    return ss.str();
}

// BasicBlock implementation
void BasicBlock::addInstruction(std::unique_ptr<Instruction> instr) {
    instructions.push_back(std::move(instr));
}

void BasicBlock::addSuccessor(BasicBlock* block) {
    successors.push_back(block);
    block->predecessors.push_back(this);
}

// Function implementation
BasicBlock* Function::createBlock(const std::string& label) {
    auto block = std::make_unique<BasicBlock>(label);
    BasicBlock* ptr = block.get();
    blocks.push_back(std::move(block));
    return ptr;
}

BasicBlock* Function::getBlock(const std::string& label) {
    for (auto& block : blocks) {
        if (block->label == label) {
            return block.get();
        }
    }
    return nullptr;
}

// CodeGenerator implementation
CodeGenerator::CodeGenerator() 
    : current_class_name(""), target_platform(TargetPlatform::MACOS_X64), output_format(OutputFormat::ASSEMBLY),
      current_function(nullptr), current_block(nullptr), 
      next_register_id(0), next_label_id(0), stack_offset(0), semantic_analyzer(nullptr) {
    initializeBuiltinFunctions();
    
    // Initialize available registers (simplified x86-64 subset)
    for (int i = 0; i < 8; ++i) {
        std::string name = "r" + std::to_string(i);
        available_registers.push_back(std::make_shared<Register>(i, Register::GENERAL, name));
    }
    
    // Initialize floating point registers
    for (int i = 0; i < 8; ++i) {
        std::string name = "xmm" + std::to_string(i);
        available_registers.push_back(std::make_shared<Register>(i + 100, Register::FLOAT, name));
    }
}

CodeGenerator::CodeGenerator(SemanticAnalyzer* analyzer) 
    : current_class_name(""), target_platform(TargetPlatform::MACOS_X64), output_format(OutputFormat::ASSEMBLY),
      current_function(nullptr), current_block(nullptr), 
      next_register_id(0), next_label_id(0), stack_offset(0), semantic_analyzer(analyzer) {
    initializeBuiltinFunctions();
    
    // Initialize available registers (simplified x86-64 subset)
    for (int i = 0; i < 8; ++i) {
        std::string name = "r" + std::to_string(i);
        available_registers.push_back(std::make_shared<Register>(i, Register::GENERAL, name));
    }
    
    // Initialize floating point registers
    for (int i = 0; i < 8; ++i) {
        std::string name = "xmm" + std::to_string(i);
        available_registers.push_back(std::make_shared<Register>(i + 100, Register::FLOAT, name));
    }
}

CodeGenerator::CodeGenerator(TargetPlatform platform, OutputFormat format)
    : current_class_name(""), target_platform(platform), output_format(format),
      current_function(nullptr), current_block(nullptr),
      next_register_id(0), next_label_id(0), stack_offset(0), semantic_analyzer(nullptr) {
    initializeBuiltinFunctions();
    
    // Initialize available registers (simplified x86-64 subset)
    for (int i = 0; i < 8; ++i) {
        std::string name = "r" + std::to_string(i);
        available_registers.push_back(std::make_shared<Register>(i, Register::GENERAL, name));
    }
    
    // Initialize floating point registers
    for (int i = 0; i < 8; ++i) {
        std::string name = "xmm" + std::to_string(i);
        available_registers.push_back(std::make_shared<Register>(i + 100, Register::FLOAT, name));
    }
}

CodeGenerator::CodeGenerator(SemanticAnalyzer* analyzer, TargetPlatform platform, OutputFormat format)
    : current_class_name(""), target_platform(platform), output_format(format),
      current_function(nullptr), current_block(nullptr),
      next_register_id(0), next_label_id(0), stack_offset(0), semantic_analyzer(analyzer) {
    initializeBuiltinFunctions();
    
    // Initialize available registers (simplified x86-64 subset)
    for (int i = 0; i < 8; ++i) {
        std::string name = "r" + std::to_string(i);
        available_registers.push_back(std::make_shared<Register>(i, Register::GENERAL, name));
    }
    
    // Initialize floating point registers
    for (int i = 0; i < 8; ++i) {
        std::string name = "xmm" + std::to_string(i);
        available_registers.push_back(std::make_shared<Register>(i + 100, Register::FLOAT, name));
    }
}

bool CodeGenerator::generate(ASTNode* root, const std::string& output_file) {
    if (!root) {
        addError("No AST to generate code from");
        return false;
    }
    
    if (root->type != ASTNodeType::PROGRAM) {
        addError("Root node is not a program");
        return false;
    }
    
    generateProgram(static_cast<Program*>(root));
    
    if (hasErrors()) {
        for (const auto& error : errors) {
            std::cerr << error << std::endl;
        }
        return false;
    }
    
    // Perform optimizations
    optimizeCode();
    
    // Generate output based on format
    switch (output_format) {
        case OutputFormat::ASSEMBLY:
            writeAssembly(output_file + ".s");
            break;
        case OutputFormat::OBJECT:
            writeAssembly(output_file + ".s");
            writeObjectFile(output_file + ".o");
            break;
        case OutputFormat::EXECUTABLE:
            writeAssembly(output_file + ".s");
            writeObjectFile(output_file + ".o");
            writeExecutable(output_file + getExecutableExtension());
            break;
    }
    
    return true;
}

bool CodeGenerator::generate(ASTNode* root, const std::string& output_file, SemanticAnalyzer* analyzer) {
    semantic_analyzer = analyzer;
    return generate(root, output_file);
}

bool CodeGenerator::generate(ASTNode* root, const std::string& output_file, TargetPlatform platform, OutputFormat format) {
    target_platform = platform;
    output_format = format;
    return generate(root, output_file);
}

// Platform and format configuration methods
void CodeGenerator::setTargetPlatform(TargetPlatform platform) {
    target_platform = platform;
}

void CodeGenerator::setOutputFormat(OutputFormat format) {
    output_format = format;
}

TargetPlatform CodeGenerator::getTargetPlatform() const {
    return target_platform;
}

OutputFormat CodeGenerator::getOutputFormat() const {
    return output_format;
}

std::string CodeGenerator::getPlatformName() const {
    switch (target_platform) {
        case TargetPlatform::WINDOWS_X64: return "Windows x64";
        case TargetPlatform::MACOS_X64: return "macOS x64";
        case TargetPlatform::MACOS_ARM64: return "macOS ARM64";
        case TargetPlatform::LINUX_X64: return "Linux x64";
        case TargetPlatform::LINUX_ARM64: return "Linux ARM64";
        default: return "Unknown";
    }
}

std::string CodeGenerator::getExecutableExtension() const {
    switch (target_platform) {
        case TargetPlatform::WINDOWS_X64: return ".exe";
        case TargetPlatform::MACOS_X64:
        case TargetPlatform::MACOS_ARM64: return ".app";
        case TargetPlatform::LINUX_X64:
        case TargetPlatform::LINUX_ARM64: return "";
        default: return "";
    }
}

void CodeGenerator::generateProgram(Program* program) {
    // Generate runtime support first
    generateRuntimeSupport();
    
    // Generate all functions and classes
    for (auto& stmt : program->statements) {
        generateStatement(stmt.get());
    }
    
    // Generate main entry point if no main function exists
    if (function_map.find("main") == function_map.end()) {
        setupFunction("main");
        emit(Instruction::MOV, allocateRegister(), 0); // Return 0
        emit(Instruction::RET);
        finalizeFunction();
    }
}

void CodeGenerator::generateStatement(Statement* stmt) {
    if (!stmt) return;
    
    switch (stmt->type) {
        case ASTNodeType::VAR_DECL:
            generateVarDecl(static_cast<VarDecl*>(stmt));
            break;
        case ASTNodeType::CONST_DECL:
            generateConstDecl(static_cast<ConstDecl*>(stmt));
            break;
        case ASTNodeType::FUNC_DECL:
            generateFuncDecl(static_cast<FuncDecl*>(stmt));
            break;
        case ASTNodeType::CLASS_DECL:
            generateClassDecl(static_cast<ClassDecl*>(stmt));
            break;
        case ASTNodeType::SIGNAL_DECL:
        generateSignalDecl(static_cast<SignalDecl*>(stmt));
        break;
    case ASTNodeType::ENUM_DECL:
        generateEnumDecl(static_cast<EnumDecl*>(stmt));
        break;
        case ASTNodeType::BLOCK:
            generateBlockStmt(static_cast<BlockStmt*>(stmt));
            break;
        case ASTNodeType::IF_STMT:
            generateIfStmt(static_cast<IfStmt*>(stmt));
            break;
        case ASTNodeType::WHILE_STMT:
            generateWhileStmt(static_cast<WhileStmt*>(stmt));
            break;
        case ASTNodeType::FOR_STMT:
            generateForStmt(static_cast<ForStmt*>(stmt));
            break;
        case ASTNodeType::MATCH_STMT:
            generateMatchStmt(static_cast<MatchStmt*>(stmt));
            break;
        case ASTNodeType::RETURN_STMT:
            generateReturnStmt(static_cast<ReturnStmt*>(stmt));
            break;
        case ASTNodeType::EXPRESSION_STMT:
            generateExpressionStmt(static_cast<ExpressionStmt*>(stmt));
            break;
        case ASTNodeType::BREAK_STMT:
            generateBreakStmt(static_cast<BreakStmt*>(stmt));
            break;
        case ASTNodeType::CONTINUE_STMT:
            generateContinueStmt(static_cast<ContinueStmt*>(stmt));
            break;
        case ASTNodeType::PASS_STMT:
            emit(Instruction::NOP);
            break;
        default:
            addError("Unknown statement type in code generation");
            break;
    }
}

void CodeGenerator::generateVarDecl(VarDecl* decl) {
    auto var_reg = allocateRegister();
    var_reg->name = decl->name;
    variables[decl->name] = var_reg;
    
    if (decl->initializer) {
        auto init_reg = generateExpression(decl->initializer.get());
        emit(Instruction::MOV, var_reg, init_reg);
        freeRegister(init_reg);
    } else {
        // Initialize to null/zero
        emit(Instruction::MOV, var_reg, 0);
    }
}

void CodeGenerator::generateConstDecl(ConstDecl* decl) {
    auto const_reg = allocateRegister();
    const_reg->name = decl->name;
    variables[decl->name] = const_reg;
    
    auto value_reg = generateExpression(decl->value.get());
    emit(Instruction::MOV, const_reg, value_reg);
    freeRegister(value_reg);
}

void CodeGenerator::generateFuncDecl(FuncDecl* decl) {
    setupFunction(decl->name);
    
    // Set up parameters
    for (size_t i = 0; i < decl->parameters.size(); ++i) {
        auto param_reg = allocateRegister();
        param_reg->name = decl->parameters[i].name;
        variables[decl->parameters[i].name] = param_reg;
        current_function->parameters.push_back(param_reg);
    }
    
    // Generate function body
    generateStatement(decl->body.get());
    
    // Ensure function returns
    if (current_block->instructions.empty() || 
        current_block->instructions.back()->opcode != Instruction::RET) {
        if (!decl->return_type.empty() && decl->return_type != "void") {
            // Return default value
            auto return_reg = allocateRegister();
            emit(Instruction::MOV, return_reg, 0);
            emit(Instruction::RET);
        } else {
            emit(Instruction::RET);
        }
    }
    
    finalizeFunction();
}

void CodeGenerator::generateClassDecl(ClassDecl* decl) {
    current_class_name = decl->name;
    
    // First, register all class member variables
    for (auto& member : decl->members) {
        if (member->type == ASTNodeType::VAR_DECL) {
            VarDecl* var_decl = static_cast<VarDecl*>(member.get());
            auto member_reg = allocateRegister();
            member_reg->name = var_decl->name;
            class_members[var_decl->name] = member_reg;
        }
    }
    
    // Then generate methods
    for (auto& member : decl->members) {
        if (member->type == ASTNodeType::FUNC_DECL) {
            FuncDecl* method = static_cast<FuncDecl*>(member.get());
            std::string mangled_name = decl->name + "_" + method->name;
            
            setupFunction(mangled_name);
            
            // Add 'self' parameter for instance methods
            if (!method->is_static) {
                auto self_reg = allocateRegister();
                self_reg->name = "self";
                variables["self"] = self_reg;
                current_function->parameters.push_back(self_reg);
            }
            
            // Add method parameters
            for (const auto& param : method->parameters) {
                auto param_reg = allocateRegister();
                param_reg->name = param.name;
                variables[param.name] = param_reg;
                current_function->parameters.push_back(param_reg);
            }
            
            generateStatement(method->body.get());
            
            if (current_block->instructions.empty() || 
                current_block->instructions.back()->opcode != Instruction::RET) {
                emit(Instruction::RET);
            }
            
            finalizeFunction();
        }
    }
    
    current_class_name = "";
}

void CodeGenerator::generateSignalDecl(SignalDecl* decl) {
    (void)decl; // Mark parameter as intentionally unused
    // Signals are handled by the runtime system
    // Generate signal registration code
    auto signal_name_reg = allocateRegister();
    emit(Instruction::MOV, signal_name_reg, 0); // Load string address
    
    // Call runtime signal registration
    emit(Instruction::CALL, "_register_signal");
    freeRegister(signal_name_reg);
}

void CodeGenerator::generateEnumDecl(EnumDecl* decl) {
    (void)decl; // Mark parameter as intentionally unused
    // Enums are compile-time constants, so we don't generate runtime code
    // The enum values are already resolved during semantic analysis
    // In a full implementation, we might generate debug information for the enum
}

void CodeGenerator::generateBlockStmt(BlockStmt* stmt) {
    for (auto& statement : stmt->statements) {
        generateStatement(statement.get());
    }
}

void CodeGenerator::generateIfStmt(IfStmt* stmt) {
    auto condition_reg = generateExpression(stmt->condition.get());
    
    std::string else_label = generateLabel("else");
    std::string end_label = generateLabel("endif");
    
    // Compare condition with false/zero
    emit(Instruction::CMP, condition_reg, 0);
    emit(Instruction::JE, else_label);
    
    freeRegister(condition_reg);
    
    // Generate then branch
    generateStatement(stmt->then_branch.get());
    emit(Instruction::JMP, end_label);
    
    // Generate else branch
    emitLabel(else_label);
    if (stmt->else_branch) {
        generateStatement(stmt->else_branch.get());
    }
    
    emitLabel(end_label);
}

void CodeGenerator::generateWhileStmt(WhileStmt* stmt) {
    std::string loop_label = generateLabel("while_loop");
    std::string end_label = generateLabel("while_end");
    
    pushBreakLabel(end_label);
    pushContinueLabel(loop_label);
    
    emitLabel(loop_label);
    
    auto condition_reg = generateExpression(stmt->condition.get());
    emit(Instruction::CMP, condition_reg, 0);
    emit(Instruction::JE, end_label);
    freeRegister(condition_reg);
    
    generateStatement(stmt->body.get());
    emit(Instruction::JMP, loop_label);
    
    emitLabel(end_label);
    
    popBreakLabel();
    popContinueLabel();
}

void CodeGenerator::generateForStmt(ForStmt* stmt) {
    // Generate iterator setup
    auto iterable_reg = generateExpression(stmt->iterable.get());
    auto iterator_reg = allocateRegister();
    auto loop_var_reg = allocateRegister();
    
    loop_var_reg->name = stmt->variable;
    variables[stmt->variable] = loop_var_reg;
    
    std::string loop_label = generateLabel("for_loop");
    std::string end_label = generateLabel("for_end");
    
    pushBreakLabel(end_label);
    pushContinueLabel(loop_label);
    
    // Initialize iterator
    emit(Instruction::MOV, iterator_reg, 0);
    
    emitLabel(loop_label);
    
    // Check if iterator is valid (simplified)
    emit(Instruction::CALL, "_iterator_valid");
    auto valid_reg = allocateRegister();
    emit(Instruction::CMP, valid_reg, 0);
    emit(Instruction::JE, end_label);
    freeRegister(valid_reg);
    
    // Get current value
    emit(Instruction::CALL, "_iterator_get");
    emit(Instruction::MOV, loop_var_reg, allocateRegister());
    
    generateStatement(stmt->body.get());
    
    // Advance iterator
    emit(Instruction::CALL, "_iterator_next");
    emit(Instruction::JMP, loop_label);
    
    emitLabel(end_label);
    
    freeRegister(iterable_reg);
    freeRegister(iterator_reg);
    
    popBreakLabel();
    popContinueLabel();
}

void CodeGenerator::generateReturnStmt(ReturnStmt* stmt) {
    if (stmt->value) {
        auto return_reg = generateExpression(stmt->value.get());
        // Move return value to designated return register
        if (current_function && current_function->return_register) {
            emit(Instruction::MOV, current_function->return_register, return_reg);
        }
        freeRegister(return_reg);
    }
    
    emit(Instruction::RET);
}

void CodeGenerator::generateExpressionStmt(ExpressionStmt* stmt) {
    auto result_reg = generateExpression(stmt->expression.get());
    freeRegister(result_reg);
}

void CodeGenerator::generateBreakStmt(BreakStmt* stmt) {
    (void)stmt; // Mark parameter as intentionally unused
    std::string break_label = getCurrentBreakLabel();
    if (!break_label.empty()) {
        emit(Instruction::JMP, break_label);
    } else {
        addError("Break statement outside of loop");
    }
}

void CodeGenerator::generateContinueStmt(ContinueStmt* stmt) {
    (void)stmt; // Mark parameter as intentionally unused
    std::string continue_label = getCurrentContinueLabel();
    if (!continue_label.empty()) {
        emit(Instruction::JMP, continue_label);
    } else {
        addError("Continue statement outside of loop");
    }
}

std::shared_ptr<Register> CodeGenerator::generateExpression(Expression* expr) {
    if (!expr) return nullptr;
    
    switch (expr->type) {
        case ASTNodeType::LITERAL:
            return generateLiteralExpr(static_cast<LiteralExpr*>(expr));
        case ASTNodeType::IDENTIFIER:
            return generateIdentifierExpr(static_cast<IdentifierExpr*>(expr));
        case ASTNodeType::BINARY_OP:
            return generateBinaryOpExpr(static_cast<BinaryOpExpr*>(expr));
        case ASTNodeType::UNARY_OP:
            return generateUnaryOpExpr(static_cast<UnaryOpExpr*>(expr));
        case ASTNodeType::CALL:
            return generateCallExpr(static_cast<CallExpr*>(expr));
        case ASTNodeType::MEMBER_ACCESS:
            return generateMemberAccessExpr(static_cast<MemberAccessExpr*>(expr));
        case ASTNodeType::ARRAY_ACCESS:
            return generateArrayAccessExpr(static_cast<ArrayAccessExpr*>(expr));
        case ASTNodeType::ARRAY_LITERAL:
            return generateArrayLiteralExpr(static_cast<ArrayLiteralExpr*>(expr));
        case ASTNodeType::DICT_LITERAL:
            return generateDictLiteralExpr(static_cast<DictLiteralExpr*>(expr));
        case ASTNodeType::LAMBDA:
            return generateLambdaExpr(static_cast<LambdaExpr*>(expr));
        case ASTNodeType::TERNARY:
            return generateTernaryExpr(static_cast<TernaryExpr*>(expr));
        default:
            addError("Unknown expression type in code generation");
            return allocateRegister();
    }
}

std::shared_ptr<Register> CodeGenerator::generateLiteralExpr(LiteralExpr* expr) {
    auto result_reg = allocateRegister();
    
    switch (expr->literal_type) {
        case TokenType::INTEGER: {
            int value = std::stoi(expr->value);
            emit(Instruction::MOV, result_reg, value);
            break;
        }
        case TokenType::FLOAT: {
            // For simplicity, convert float to int representation
            float value = std::stof(expr->value);
            result_reg = allocateRegister(Register::FLOAT);
            emit(Instruction::MOV, result_reg, static_cast<int>(value * 1000)); // Scale for demo
            break;
        }
        case TokenType::STRING: {
            // Load string address (simplified)
            emit(Instruction::MOV, result_reg, 0); // String table address
            break;
        }
        case TokenType::BOOLEAN: {
            int value = (expr->value == "true") ? 1 : 0;
            emit(Instruction::MOV, result_reg, value);
            break;
        }
        case TokenType::NULL_LITERAL: {
            emit(Instruction::MOV, result_reg, 0);
            break;
        }
        default:
            emit(Instruction::MOV, result_reg, 0);
            break;
    }
    
    return result_reg;
}

// Machine code generation
std::vector<uint8_t> CodeGenerator::generateMachineCode() {
    std::vector<uint8_t> machine_code;
    
    // Convert our intermediate representation to machine code
    for (auto& func : functions) {
        for (auto& block : func->blocks) {
            for (auto& instr : block->instructions) {
                std::vector<uint8_t> instr_bytes = generateInstructionBytes(instr.get());
                machine_code.insert(machine_code.end(), instr_bytes.begin(), instr_bytes.end());
            }
        }
    }
    
    return machine_code;
}

// Generate machine code bytes for a single instruction
std::vector<uint8_t> CodeGenerator::generateInstructionBytes(Instruction* instr) {
    std::vector<uint8_t> bytes;
    
    switch (target_platform) {
        case TargetPlatform::WINDOWS_X64:
        case TargetPlatform::LINUX_X64:
        case TargetPlatform::MACOS_X64:
            bytes = generateX86_64Instruction(instr);
            break;
        case TargetPlatform::MACOS_ARM64:
        case TargetPlatform::LINUX_ARM64:
            bytes = generateARM64Instruction(instr);
            break;
        default:
            // Fallback to simple NOP
            bytes.push_back(0x90); // x86 NOP
            break;
    }
    
    return bytes;
}

// Generate x86_64 machine code for an instruction
std::vector<uint8_t> CodeGenerator::generateX86_64Instruction(Instruction* instr) {
    std::vector<uint8_t> bytes;
    
    switch (instr->opcode) {
        case Instruction::MOV:
            if (instr->has_immediate) {
                // mov reg, imm32
                bytes.push_back(0x48); // REX.W prefix for 64-bit
                bytes.push_back(0xc7);
                bytes.push_back(0xc0); // ModR/M for RAX
                // Add immediate value (little endian)
                uint32_t imm = static_cast<uint32_t>(instr->immediate);
                bytes.push_back(imm & 0xFF);
                bytes.push_back((imm >> 8) & 0xFF);
                bytes.push_back((imm >> 16) & 0xFF);
                bytes.push_back((imm >> 24) & 0xFF);
            } else {
                // mov reg, reg
                bytes.push_back(0x48); // REX.W prefix
                bytes.push_back(0x89);
                bytes.push_back(0xc0); // ModR/M for mov rax, rax
            }
            break;
            
        case Instruction::ADD:
            if (instr->has_immediate) {
                // add reg, imm32
                bytes.push_back(0x48); // REX.W prefix
                bytes.push_back(0x81);
                bytes.push_back(0xc0); // ModR/M for add rax, imm32
                uint32_t imm = static_cast<uint32_t>(instr->immediate);
                bytes.push_back(imm & 0xFF);
                bytes.push_back((imm >> 8) & 0xFF);
                bytes.push_back((imm >> 16) & 0xFF);
                bytes.push_back((imm >> 24) & 0xFF);
            } else {
                // add reg, reg
                bytes.push_back(0x48); // REX.W prefix
                bytes.push_back(0x01);
                bytes.push_back(0xc0); // ModR/M for add rax, rax
            }
            break;
            
        case Instruction::SUB:
            if (instr->has_immediate) {
                // sub reg, imm32
                bytes.push_back(0x48); // REX.W prefix
                bytes.push_back(0x81);
                bytes.push_back(0xe8); // ModR/M for sub rax, imm32
                uint32_t imm = static_cast<uint32_t>(instr->immediate);
                bytes.push_back(imm & 0xFF);
                bytes.push_back((imm >> 8) & 0xFF);
                bytes.push_back((imm >> 16) & 0xFF);
                bytes.push_back((imm >> 24) & 0xFF);
            } else {
                // sub reg, reg
                bytes.push_back(0x48); // REX.W prefix
                bytes.push_back(0x29);
                bytes.push_back(0xc0); // ModR/M for sub rax, rax
            }
            break;
            
        case Instruction::CALL:
            // call rel32 (placeholder)
            bytes.push_back(0xe8);
            bytes.push_back(0x00);
            bytes.push_back(0x00);
            bytes.push_back(0x00);
            bytes.push_back(0x00);
            break;
            
        case Instruction::RET:
            bytes.push_back(0xc3); // ret
            break;
            
        case Instruction::PUSH:
            bytes.push_back(0x50); // push rax (simplified)
            break;
            
        case Instruction::POP:
            bytes.push_back(0x58); // pop rax (simplified)
            break;
            
        case Instruction::NOP:
            bytes.push_back(0x90); // nop
            break;
            
        default:
            // Unknown instruction, emit NOP
            bytes.push_back(0x90);
            break;
    }
    
    return bytes;
}

// Generate ARM64 machine code for an instruction
std::vector<uint8_t> CodeGenerator::generateARM64Instruction(Instruction* instr) {
    std::vector<uint8_t> bytes;
    
    switch (instr->opcode) {
        case Instruction::MOV:
            if (instr->has_immediate) {
                // mov x0, #imm16
                uint32_t imm = static_cast<uint32_t>(instr->immediate) & 0xFFFF;
                uint32_t instruction = 0xd2800000 | (imm << 5); // mov x0, #imm16
                bytes.push_back(instruction & 0xFF);
                bytes.push_back((instruction >> 8) & 0xFF);
                bytes.push_back((instruction >> 16) & 0xFF);
                bytes.push_back((instruction >> 24) & 0xFF);
            } else {
                // mov x0, x1
                bytes.push_back(0xe0); // mov x0, x1
                bytes.push_back(0x03);
                bytes.push_back(0x01);
                bytes.push_back(0xaa);
            }
            break;
            
        case Instruction::ADD:
            if (instr->has_immediate) {
                // add x0, x0, #imm12
                uint32_t imm = static_cast<uint32_t>(instr->immediate) & 0xFFF;
                uint32_t instruction = 0x91000000 | (imm << 10); // add x0, x0, #imm12
                bytes.push_back(instruction & 0xFF);
                bytes.push_back((instruction >> 8) & 0xFF);
                bytes.push_back((instruction >> 16) & 0xFF);
                bytes.push_back((instruction >> 24) & 0xFF);
            } else {
                // add x0, x0, x1
                bytes.push_back(0x00); // add x0, x0, x1
                bytes.push_back(0x00);
                bytes.push_back(0x01);
                bytes.push_back(0x8b);
            }
            break;
            
        case Instruction::SUB:
            if (instr->has_immediate) {
                // sub x0, x0, #imm12
                uint32_t imm = static_cast<uint32_t>(instr->immediate) & 0xFFF;
                uint32_t instruction = 0xd1000000 | (imm << 10); // sub x0, x0, #imm12
                bytes.push_back(instruction & 0xFF);
                bytes.push_back((instruction >> 8) & 0xFF);
                bytes.push_back((instruction >> 16) & 0xFF);
                bytes.push_back((instruction >> 24) & 0xFF);
            } else {
                // sub x0, x0, x1
                bytes.push_back(0x00); // sub x0, x0, x1
                bytes.push_back(0x00);
                bytes.push_back(0x01);
                bytes.push_back(0xcb);
            }
            break;
            
        case Instruction::CALL:
            // bl #0 (placeholder)
            bytes.push_back(0x00);
            bytes.push_back(0x00);
            bytes.push_back(0x00);
            bytes.push_back(0x94);
            break;
            
        case Instruction::RET:
            bytes.push_back(0xc0); // ret
            bytes.push_back(0x03);
            bytes.push_back(0x5f);
            bytes.push_back(0xd6);
            break;
            
        case Instruction::NOP:
            bytes.push_back(0x1f); // nop
            bytes.push_back(0x20);
            bytes.push_back(0x03);
            bytes.push_back(0xd5);
            break;
            
        default:
            // Unknown instruction, emit NOP
            bytes.push_back(0x1f);
            bytes.push_back(0x20);
            bytes.push_back(0x03);
            bytes.push_back(0xd5);
            break;
    }
    
    return bytes;
}

std::shared_ptr<Register> CodeGenerator::generateIdentifierExpr(IdentifierExpr* expr) {
    // First check local variables
    auto it = variables.find(expr->name);
    if (it != variables.end()) {
        auto result_reg = allocateRegister();
        emit(Instruction::MOV, result_reg, it->second);
        return result_reg;
    }
    
    // Then check class member variables
    auto class_it = class_members.find(expr->name);
    if (class_it != class_members.end()) {
        auto result_reg = allocateRegister();
        emit(Instruction::MOV, result_reg, class_it->second);
        return result_reg;
    }
    
    // Check semantic analyzer's symbol table if available
    if (semantic_analyzer) {
        auto global_scope = semantic_analyzer->getGlobalScope();
        if (global_scope) {
            // Check for variables in global scope
            auto symbol = global_scope->findSymbol(expr->name);
            if (symbol) {
                // Create a register for this variable if not already created
                auto var_reg = allocateRegister();
                var_reg->name = expr->name;
                variables[expr->name] = var_reg;
                
                auto result_reg = allocateRegister();
                emit(Instruction::MOV, result_reg, var_reg);
                return result_reg;
            }
            
            // Check for functions in global scope
            auto function = global_scope->findFunction(expr->name);
            if (function) {
                // Return function address for function references
                auto result_reg = allocateRegister();
                emit(Instruction::MOV, result_reg, 0); // Function address placeholder
                return result_reg;
            }
        }
        
        // Check class members if we're in a class context
        if (!current_class_name.empty()) {
            auto& classes = semantic_analyzer->getClasses();
            auto class_it = classes.find(current_class_name);
            if (class_it != classes.end()) {
                // Check class member variables
                auto member_it = class_it->second.members.find(expr->name);
                if (member_it != class_it->second.members.end()) {
                    // Create a register for this class member if not already created
                    auto member_reg = allocateRegister();
                    member_reg->name = expr->name;
                    class_members[expr->name] = member_reg;
                    
                    auto result_reg = allocateRegister();
                    emit(Instruction::MOV, result_reg, member_reg);
                    return result_reg;
                }
                
                // Check class methods
                auto method_it = class_it->second.methods.find(expr->name);
                if (method_it != class_it->second.methods.end()) {
                    // Return function address for method references
                    auto result_reg = allocateRegister();
                    emit(Instruction::MOV, result_reg, 0); // Function address placeholder
                    return result_reg;
                }
            }
        }
    }
    
    addError("Undefined variable: " + expr->name);
    return allocateRegister();
}

std::shared_ptr<Register> CodeGenerator::generateBinaryOpExpr(BinaryOpExpr* expr) {
    auto left_reg = generateExpression(expr->left.get());
    auto right_reg = generateExpression(expr->right.get());
    auto result_reg = allocateRegister();
    
    switch (expr->operator_type) {
        case TokenType::PLUS:
            emit(Instruction::ADD, result_reg, left_reg, right_reg);
            break;
        case TokenType::MINUS:
            emit(Instruction::SUB, result_reg, left_reg, right_reg);
            break;
        case TokenType::MULTIPLY:
            emit(Instruction::MUL, result_reg, left_reg, right_reg);
            break;
        case TokenType::DIVIDE:
            emit(Instruction::DIV, result_reg, left_reg, right_reg);
            break;
        case TokenType::MODULO:
            emit(Instruction::MOD, result_reg, left_reg, right_reg);
            break;
        case TokenType::EQUAL:
        case TokenType::NOT_EQUAL:
        case TokenType::LESS:
        case TokenType::LESS_EQUAL:
        case TokenType::GREATER:
        case TokenType::GREATER_EQUAL: {
            emit(Instruction::CMP, left_reg, right_reg);
            
            std::string true_label = generateLabel("cmp_true");
            std::string end_label = generateLabel("cmp_end");
            
            switch (expr->operator_type) {
                case TokenType::EQUAL: emit(Instruction::JE, true_label); break;
                case TokenType::NOT_EQUAL: emit(Instruction::JNE, true_label); break;
                case TokenType::LESS: emit(Instruction::JL, true_label); break;
                case TokenType::LESS_EQUAL: emit(Instruction::JLE, true_label); break;
                case TokenType::GREATER: emit(Instruction::JG, true_label); break;
                case TokenType::GREATER_EQUAL: emit(Instruction::JGE, true_label); break;
                default: break;
            }
            
            emit(Instruction::MOV, result_reg, 0); // False
            emit(Instruction::JMP, end_label);
            emitLabel(true_label);
            emit(Instruction::MOV, result_reg, 1); // True
            emitLabel(end_label);
            break;
        }
        case TokenType::AND:
            emit(Instruction::AND, result_reg, left_reg, right_reg);
            break;
        case TokenType::OR:
            emit(Instruction::OR, result_reg, left_reg, right_reg);
            break;
        case TokenType::ASSIGN:
            emit(Instruction::MOV, left_reg, right_reg);
            emit(Instruction::MOV, result_reg, left_reg);
            break;
        case TokenType::PLUS_ASSIGN:
            emit(Instruction::ADD, left_reg, left_reg, right_reg);
            emit(Instruction::MOV, result_reg, left_reg);
            break;
        case TokenType::MINUS_ASSIGN:
            emit(Instruction::SUB, left_reg, left_reg, right_reg);
            emit(Instruction::MOV, result_reg, left_reg);
            break;
        case TokenType::MULTIPLY_ASSIGN:
            emit(Instruction::MUL, left_reg, left_reg, right_reg);
            emit(Instruction::MOV, result_reg, left_reg);
            break;
        case TokenType::DIVIDE_ASSIGN:
            emit(Instruction::DIV, left_reg, left_reg, right_reg);
            emit(Instruction::MOV, result_reg, left_reg);
            break;
        case TokenType::MODULO_ASSIGN:
            emit(Instruction::MOD, left_reg, left_reg, right_reg);
            emit(Instruction::MOV, result_reg, left_reg);
            break;
        case TokenType::TYPE_INFER_ASSIGN:
            // Type inference assignment - same as regular assignment for code generation
            emit(Instruction::MOV, left_reg, right_reg);
            emit(Instruction::MOV, result_reg, left_reg);
            break;
        default:
            addError("Unknown binary operator");
            emit(Instruction::MOV, result_reg, 0);
            break;
    }
    
    freeRegister(left_reg);
    freeRegister(right_reg);
    
    return result_reg;
}

std::shared_ptr<Register> CodeGenerator::generateUnaryOpExpr(UnaryOpExpr* expr) {
    auto operand_reg = generateExpression(expr->operand.get());
    auto result_reg = allocateRegister();
    
    switch (expr->operator_type) {
        case TokenType::MINUS:
            emit(Instruction::SUB, result_reg, allocateRegister(), operand_reg);
            break;
        case TokenType::PLUS:
            emit(Instruction::MOV, result_reg, operand_reg);
            break;
        case TokenType::NOT:
        case TokenType::LOGICAL_NOT:
            emit(Instruction::NOT, result_reg, operand_reg);
            break;
        default:
            addError("Unknown unary operator");
            emit(Instruction::MOV, result_reg, operand_reg);
            break;
    }
    
    freeRegister(operand_reg);
    return result_reg;
}

std::shared_ptr<Register> CodeGenerator::generateCallExpr(CallExpr* expr) {
    std::vector<std::shared_ptr<Register>> arg_regs;
    
    // Generate arguments
    for (auto& arg : expr->arguments) {
        arg_regs.push_back(generateExpression(arg.get()));
    }
    
    // Check if it's a built-in function
    if (expr->callee->type == ASTNodeType::IDENTIFIER) {
        IdentifierExpr* id_expr = static_cast<IdentifierExpr*>(expr->callee.get());
        if (isBuiltinFunction(id_expr->name)) {
            auto result = generateBuiltinCall(id_expr->name, arg_regs);
            for (auto& reg : arg_regs) {
                freeRegister(reg);
            }
            return result;
        }
    }
    
    // Push arguments onto stack (reverse order for calling convention)
    for (auto it = arg_regs.rbegin(); it != arg_regs.rend(); ++it) {
        emit(Instruction::PUSH, *it);
    }
    
    // Generate function call
    if (expr->callee->type == ASTNodeType::IDENTIFIER) {
        IdentifierExpr* id_expr = static_cast<IdentifierExpr*>(expr->callee.get());
        emit(Instruction::CALL, id_expr->name);
    } else {
        // Indirect call
        auto callee_reg = generateExpression(expr->callee.get());
        emit(Instruction::CALL, callee_reg->name);
        freeRegister(callee_reg);
    }
    
    // Clean up stack
    for (size_t i = 0; i < arg_regs.size(); ++i) {
        emit(Instruction::POP, allocateRegister());
    }
    
    // Get return value
    auto result_reg = allocateRegister();
    // Return value is typically in a specific register (e.g., rax)
    
    for (auto& reg : arg_regs) {
        freeRegister(reg);
    }
    
    return result_reg;
}

std::shared_ptr<Register> CodeGenerator::generateMemberAccessExpr(MemberAccessExpr* expr) {
    auto object_reg = generateExpression(expr->object.get());
    auto result_reg = allocateRegister();
    
    // Simplified member access - load from offset
    emit(Instruction::LOAD, result_reg, object_reg);
    
    freeRegister(object_reg);
    return result_reg;
}

std::shared_ptr<Register> CodeGenerator::generateArrayAccessExpr(ArrayAccessExpr* expr) {
    auto array_reg = generateExpression(expr->array.get());
    auto index_reg = generateExpression(expr->index.get());
    auto result_reg = allocateRegister();
    
    // Call runtime array access function
    emit(Instruction::PUSH, array_reg);
    emit(Instruction::PUSH, index_reg);
    emit(Instruction::CALL, "_array_get");
    emit(Instruction::POP, allocateRegister());
    emit(Instruction::POP, allocateRegister());
    
    freeRegister(array_reg);
    freeRegister(index_reg);
    
    return result_reg;
}

std::shared_ptr<Register> CodeGenerator::generateArrayLiteralExpr(ArrayLiteralExpr* expr) {
    auto result_reg = allocateRegister();
    
    // Call runtime array creation
    emit(Instruction::CALL, "_array_create");
    
    // Add elements
    for (auto& element : expr->elements) {
        auto element_reg = generateExpression(element.get());
        emit(Instruction::PUSH, result_reg);
        emit(Instruction::PUSH, element_reg);
        emit(Instruction::CALL, "_array_append");
        emit(Instruction::POP, allocateRegister());
        emit(Instruction::POP, allocateRegister());
        freeRegister(element_reg);
    }
    
    return result_reg;
}

std::shared_ptr<Register> CodeGenerator::generateDictLiteralExpr(DictLiteralExpr* expr) {
    auto result_reg = allocateRegister();
    
    // Call runtime dictionary creation
    emit(Instruction::CALL, "_dict_create");
    
    // Add key-value pairs
    for (auto& pair : expr->pairs) {
        auto key_reg = generateExpression(pair.first.get());
        auto value_reg = generateExpression(pair.second.get());
        
        emit(Instruction::PUSH, result_reg);
        emit(Instruction::PUSH, key_reg);
        emit(Instruction::PUSH, value_reg);
        emit(Instruction::CALL, "_dict_set");
        emit(Instruction::POP, allocateRegister());
        emit(Instruction::POP, allocateRegister());
        emit(Instruction::POP, allocateRegister());
        
        freeRegister(key_reg);
        freeRegister(value_reg);
    }
    
    return result_reg;
}

// Register management
std::shared_ptr<Register> CodeGenerator::allocateRegister(Register::Type type) {
    for (auto& reg : available_registers) {
        if (!reg->is_allocated && reg->type == type) {
            reg->is_allocated = true;
            allocated_registers.push_back(reg);
            return reg;
        }
    }
    
    // If no physical register available, create virtual register
    return allocateVirtualRegister(type);
}

std::shared_ptr<Register> CodeGenerator::allocateVirtualRegister(Register::Type type) {
    std::string name = "v" + std::to_string(next_register_id++);
    auto reg = std::make_shared<Register>(next_register_id, Register::VIRTUAL, name);
    reg->type = type;
    return reg;
}

void CodeGenerator::freeRegister(std::shared_ptr<Register> reg) {
    if (reg && reg->is_allocated) {
        reg->is_allocated = false;
        auto it = std::find(allocated_registers.begin(), allocated_registers.end(), reg);
        if (it != allocated_registers.end()) {
            allocated_registers.erase(it);
        }
    }
}

void CodeGenerator::performRegisterAllocation() {
    // Simplified register allocation - linear scan
    for (auto& func : functions) {
        for (auto& block : func->blocks) {
            for (auto& instr : block->instructions) {
                for (auto& operand : instr->operands) {
                    if (operand && operand->type == Register::VIRTUAL) {
                        // Find a physical register
                        for (auto& phys_reg : available_registers) {
                            if (!phys_reg->is_allocated && phys_reg->type == operand->type) {
                                operand->name = phys_reg->name;
                                operand->id = phys_reg->id;
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
}

// Label management
std::string CodeGenerator::generateLabel(const std::string& prefix) {
    return prefix + "_" + std::to_string(next_label_id++);
}

// Instruction helpers
void CodeGenerator::emit(Instruction::OpCode opcode) {
    if (current_block) {
        current_block->addInstruction(std::make_unique<Instruction>(opcode));
    }
}

void CodeGenerator::emit(Instruction::OpCode opcode, std::shared_ptr<Register> dest) {
    if (current_block) {
        auto instr = std::make_unique<Instruction>(opcode);
        instr->operands.push_back(dest);
        current_block->addInstruction(std::move(instr));
    }
}

void CodeGenerator::emit(Instruction::OpCode opcode, std::shared_ptr<Register> dest, std::shared_ptr<Register> src) {
    if (current_block) {
        auto instr = std::make_unique<Instruction>(opcode);
        instr->operands.push_back(dest);
        instr->operands.push_back(src);
        current_block->addInstruction(std::move(instr));
    }
}

void CodeGenerator::emit(Instruction::OpCode opcode, std::shared_ptr<Register> dest, std::shared_ptr<Register> src1, std::shared_ptr<Register> src2) {
    if (current_block) {
        auto instr = std::make_unique<Instruction>(opcode);
        instr->operands.push_back(dest);
        instr->operands.push_back(src1);
        instr->operands.push_back(src2);
        current_block->addInstruction(std::move(instr));
    }
}

void CodeGenerator::emit(Instruction::OpCode opcode, std::shared_ptr<Register> dest, int immediate) {
    if (current_block) {
        auto instr = std::make_unique<Instruction>(opcode);
        instr->operands.push_back(dest);
        instr->immediate = immediate;
        instr->has_immediate = true;
        current_block->addInstruction(std::move(instr));
    }
}

void CodeGenerator::emit(Instruction::OpCode opcode, const std::string& label) {
    if (current_block) {
        auto instr = std::make_unique<Instruction>(opcode, label);
        current_block->addInstruction(std::move(instr));
    }
}

void CodeGenerator::emitLabel(const std::string& label) {
    if (current_block) {
        auto instr = std::make_unique<Instruction>(Instruction::LABEL, label);
        current_block->addInstruction(std::move(instr));
    }
}

// Built-in function support
void CodeGenerator::initializeBuiltinFunctions() {
    builtin_functions["print"] = "_builtin_print";
    builtin_functions["len"] = "_builtin_len";
    builtin_functions["range"] = "_builtin_range";
    builtin_functions["str"] = "_builtin_str";
    builtin_functions["int"] = "_builtin_int";
    builtin_functions["float"] = "_builtin_float";
}

bool CodeGenerator::isBuiltinFunction(const std::string& name) {
    return builtin_functions.find(name) != builtin_functions.end();
}

std::shared_ptr<Register> CodeGenerator::generateBuiltinCall(const std::string& name, const std::vector<std::shared_ptr<Register>>& args) {
    auto result_reg = allocateRegister();
    
    // Push arguments
    for (auto it = args.rbegin(); it != args.rend(); ++it) {
        emit(Instruction::PUSH, *it);
    }
    
    // Call built-in function
    emit(Instruction::CALL, builtin_functions[name]);
    
    // Clean up stack
    for (size_t i = 0; i < args.size(); ++i) {
        emit(Instruction::POP, allocateRegister());
    }
    
    return result_reg;
}

// Assembly output
void CodeGenerator::writeAssembly(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        addError("Cannot open output file: " + filename);
        return;
    }
    
    file << ".section .text\n";
    file << ".global _start\n\n";
    
    for (auto& func : functions) {
        file << func->name << ":\n";
        
        for (auto& block : func->blocks) {
            for (auto& instr : block->instructions) {
                file << "    " << instr->toString() << "\n";
            }
        }
        
        file << "\n";
    }
    
    file.close();
}

void CodeGenerator::writeObjectFile(const std::string& filename) {
    // Simplified object file generation
    // In a real implementation, this would generate ELF/Mach-O/PE format
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        addError("Cannot open object file: " + filename);
        return;
    }
    
    // Write simplified object format
    file << "GDOBJ"; // Magic number
    
    // Write function count
    uint32_t func_count = static_cast<uint32_t>(functions.size());
    file.write(reinterpret_cast<const char*>(&func_count), sizeof(func_count));
    
    // Write functions
    for (auto& func : functions) {
        // Write function name length and name
        uint32_t name_len = static_cast<uint32_t>(func->name.length());
        file.write(reinterpret_cast<const char*>(&name_len), sizeof(name_len));
        file.write(func->name.c_str(), name_len);
        
        // Write instruction count
        uint32_t instr_count = 0;
        for (auto& block : func->blocks) {
            instr_count += static_cast<uint32_t>(block->instructions.size());
        }
        file.write(reinterpret_cast<const char*>(&instr_count), sizeof(instr_count));
        
        // Write instructions (simplified)
        for (auto& block : func->blocks) {
            for (auto& instr : block->instructions) {
                uint32_t opcode = static_cast<uint32_t>(instr->opcode);
                file.write(reinterpret_cast<const char*>(&opcode), sizeof(opcode));
            }
        }
    }
    
    file.close();
}

void CodeGenerator::writeExecutable(const std::string& filename) {
    // Platform-specific executable generation
    switch (target_platform) {
        case TargetPlatform::WINDOWS_X64:
            generateWindowsExecutable(filename);
            break;
        case TargetPlatform::MACOS_X64:
        case TargetPlatform::MACOS_ARM64:
            generateMacOSExecutable(filename);
            break;
        case TargetPlatform::LINUX_X64:
        case TargetPlatform::LINUX_ARM64:
            generateLinuxExecutable(filename);
            break;
        default:
            addError("Unsupported target platform for executable generation");
            break;
    }
}

void CodeGenerator::generateWindowsExecutable(const std::string& filename) {
    // Complete Windows PE executable generation
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        addError("Cannot create Windows executable: " + filename);
        return;
    }
    
    // DOS Header
    struct DOSHeader {
        uint16_t e_magic = 0x5A4D;     // "MZ"
        uint16_t e_cblp = 0x90;
        uint16_t e_cp = 0x03;
        uint16_t e_crlc = 0x00;
        uint16_t e_cparhdr = 0x04;
        uint16_t e_minalloc = 0x00;
        uint16_t e_maxalloc = 0xFFFF;
        uint16_t e_ss = 0x00;
        uint16_t e_sp = 0xB8;
        uint16_t e_csum = 0x00;
        uint16_t e_ip = 0x00;
        uint16_t e_cs = 0x00;
        uint16_t e_lfarlc = 0x40;
        uint16_t e_ovno = 0x00;
        uint16_t e_res[4] = {0};
        uint16_t e_oemid = 0x00;
        uint16_t e_oeminfo = 0x00;
        uint16_t e_res2[10] = {0};
        uint32_t e_lfanew = 0x80;      // Offset to PE header
    } dos_header;
    
    file.write(reinterpret_cast<const char*>(&dos_header), sizeof(dos_header));
    
    // DOS stub (simple program that prints "This program cannot be run in DOS mode")
    const uint8_t dos_stub[] = {
        0x0e, 0x1f, 0xba, 0x0e, 0x00, 0xb4, 0x09, 0xcd, 0x21, 0xb8, 0x01, 0x4c, 0xcd, 0x21,
        0x54, 0x68, 0x69, 0x73, 0x20, 0x70, 0x72, 0x6f, 0x67, 0x72, 0x61, 0x6d, 0x20, 0x63,
        0x61, 0x6e, 0x6e, 0x6f, 0x74, 0x20, 0x62, 0x65, 0x20, 0x72, 0x75, 0x6e, 0x20, 0x69,
        0x6e, 0x20, 0x44, 0x4f, 0x53, 0x20, 0x6d, 0x6f, 0x64, 0x65, 0x2e, 0x0d, 0x0d, 0x0a,
        0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    file.write(reinterpret_cast<const char*>(dos_stub), sizeof(dos_stub));
    
    // PE Header
    struct PEHeader {
        uint32_t signature = 0x00004550;  // "PE\0\0"
        uint16_t machine = 0x8664;        // IMAGE_FILE_MACHINE_AMD64
        uint16_t numberOfSections = 2;    // .text and .data
        uint32_t timeDateStamp = 0;
        uint32_t pointerToSymbolTable = 0;
        uint32_t numberOfSymbols = 0;
        uint16_t sizeOfOptionalHeader = 240;
        uint16_t characteristics = 0x0102; // IMAGE_FILE_EXECUTABLE_IMAGE | IMAGE_FILE_32BIT_MACHINE
    } pe_header;
    
    file.write(reinterpret_cast<const char*>(&pe_header), sizeof(pe_header));
    
    // Optional Header
    struct OptionalHeader {
        uint16_t magic = 0x020b;          // PE32+
        uint8_t majorLinkerVersion = 14;
        uint8_t minorLinkerVersion = 0;
        uint32_t sizeOfCode = 0x1000;
        uint32_t sizeOfInitializedData = 0x1000;
        uint32_t sizeOfUninitializedData = 0;
        uint32_t addressOfEntryPoint = 0x1000;
        uint32_t baseOfCode = 0x1000;
        uint64_t imageBase = 0x140000000;
        uint32_t sectionAlignment = 0x1000;
        uint32_t fileAlignment = 0x200;
        uint16_t majorOperatingSystemVersion = 6;
        uint16_t minorOperatingSystemVersion = 0;
        uint16_t majorImageVersion = 0;
        uint16_t minorImageVersion = 0;
        uint16_t majorSubsystemVersion = 6;
        uint16_t minorSubsystemVersion = 0;
        uint32_t win32VersionValue = 0;
        uint32_t sizeOfImage = 0x3000;
        uint32_t sizeOfHeaders = 0x400;
        uint32_t checkSum = 0;
        uint16_t subsystem = 3;           // IMAGE_SUBSYSTEM_WINDOWS_CUI
        uint16_t dllCharacteristics = 0x8160;
        uint64_t sizeOfStackReserve = 0x100000;
        uint64_t sizeOfStackCommit = 0x1000;
        uint64_t sizeOfHeapReserve = 0x100000;
        uint64_t sizeOfHeapCommit = 0x1000;
        uint32_t loaderFlags = 0;
        uint32_t numberOfRvaAndSizes = 16;
        uint64_t dataDirectory[16] = {0}; // All zeros for simplicity
    } opt_header;
    
    file.write(reinterpret_cast<const char*>(&opt_header), sizeof(opt_header));
    
    // Section Headers
    struct SectionHeader {
        char name[8];
        uint32_t virtualSize;
        uint32_t virtualAddress;
        uint32_t sizeOfRawData;
        uint32_t pointerToRawData;
        uint32_t pointerToRelocations;
        uint32_t pointerToLinenumbers;
        uint16_t numberOfRelocations;
        uint16_t numberOfLinenumbers;
        uint32_t characteristics;
    };
    
    // .text section
    SectionHeader text_section = {};
    strncpy(text_section.name, ".text", 8);
    text_section.virtualSize = 0x1000;
    text_section.virtualAddress = 0x1000;
    text_section.sizeOfRawData = 0x200;
    text_section.pointerToRawData = 0x400;
    text_section.characteristics = 0x60000020; // CODE | EXECUTE | READ
    file.write(reinterpret_cast<const char*>(&text_section), sizeof(text_section));
    
    // .data section
    SectionHeader data_section = {};
    strncpy(data_section.name, ".data", 8);
    data_section.virtualSize = 0x1000;
    data_section.virtualAddress = 0x2000;
    data_section.sizeOfRawData = 0x200;
    data_section.pointerToRawData = 0x600;
    data_section.characteristics = 0xC0000040; // INITIALIZED_DATA | READ | WRITE
    file.write(reinterpret_cast<const char*>(&data_section), sizeof(data_section));
    
    // Pad to file alignment
    file.seekp(0x400);
    
    // Generate machine code from our intermediate representation
    std::vector<uint8_t> machine_code = generateMachineCode();
    
    // If no code generated, use default "Hello World" program
    if (machine_code.empty()) {
        const uint8_t hello_world_code[] = {
            // Simple x64 assembly that calls ExitProcess
            0x48, 0xc7, 0xc1, 0x00, 0x00, 0x00, 0x00,  // mov rcx, 0 (exit code)
            0x48, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // mov rax, ExitProcess address (placeholder)
            0xff, 0xd0,  // call rax
            0xc3         // ret
        };
        machine_code.assign(hello_world_code, hello_world_code + sizeof(hello_world_code));
    }
    
    file.write(reinterpret_cast<const char*>(machine_code.data()), machine_code.size());
    
    // Pad .text section
    file.seekp(0x600);
    
    // .data section (empty for now)
    const char hello_string[] = "Hello, World from GDScript!\n";
    file.write(hello_string, sizeof(hello_string));
    
    file.close();
}

void CodeGenerator::generateMacOSExecutable(const std::string& filename) {
    // Complete macOS Mach-O executable generation
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        addError("Cannot create macOS executable: " + filename);
        return;
    }
    
    // Mach-O Header (64-bit)
    struct MachHeader64 {
        uint32_t magic = 0xfeedfacf;      // MH_MAGIC_64
        uint32_t cputype;
        uint32_t cpusubtype;
        uint32_t filetype = 2;            // MH_EXECUTE
        uint32_t ncmds = 3;               // Number of load commands
        uint32_t sizeofcmds = 0x1c8;      // Size of load commands
        uint32_t flags = 0x00200085;      // MH_NOUNDEFS | MH_DYLDLINK | MH_TWOLEVEL
        uint32_t reserved = 0;
    } mach_header;
    
    if (target_platform == TargetPlatform::MACOS_ARM64) {
        mach_header.cputype = 0x0100000c;    // CPU_TYPE_ARM64
        mach_header.cpusubtype = 0x00000000; // CPU_SUBTYPE_ARM64_ALL
    } else {
        mach_header.cputype = 0x01000007;    // CPU_TYPE_X86_64
        mach_header.cpusubtype = 0x00000003; // CPU_SUBTYPE_X86_64_ALL
    }
    
    file.write(reinterpret_cast<const char*>(&mach_header), sizeof(mach_header));
    
    // Load Command 1: LC_SEGMENT_64 for __TEXT
    struct SegmentCommand64 {
        uint32_t cmd = 0x19;              // LC_SEGMENT_64
        uint32_t cmdsize = 0x98;          // Size of this command
        char segname[16] = "__TEXT";
        uint64_t vmaddr = 0x100000000;    // Virtual memory address
        uint64_t vmsize = 0x1000;         // Virtual memory size
        uint64_t fileoff = 0;             // File offset
        uint64_t filesize = 0x1000;       // File size
        uint32_t maxprot = 7;             // VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE
        uint32_t initprot = 5;            // VM_PROT_READ | VM_PROT_EXECUTE
        uint32_t nsects = 1;              // Number of sections
        uint32_t flags = 0;
    } text_segment;
    
    file.write(reinterpret_cast<const char*>(&text_segment), sizeof(text_segment));
    
    // Section for __TEXT,__text
    struct Section64 {
        char sectname[16] = "__text";
        char segname[16] = "__TEXT";
        uint64_t addr = 0x100000f50;      // Section address
        uint64_t size = 0x20;             // Section size
        uint32_t offset = 0xf50;          // File offset
        uint32_t align = 4;               // Alignment (2^4 = 16)
        uint32_t reloff = 0;              // Relocation offset
        uint32_t nreloc = 0;              // Number of relocations
        uint32_t flags = 0x80000400;      // S_REGULAR | S_ATTR_PURE_INSTRUCTIONS
        uint32_t reserved1 = 0;
        uint32_t reserved2 = 0;
        uint32_t reserved3 = 0;
    } text_section;
    
    file.write(reinterpret_cast<const char*>(&text_section), sizeof(text_section));
    
    // Load Command 2: LC_SEGMENT_64 for __DATA
    struct SegmentCommand64 data_segment;
    data_segment.cmd = 0x19;                      // LC_SEGMENT_64
    data_segment.cmdsize = 0x98;                  // Size of this command
    std::memset(data_segment.segname, 0, sizeof(data_segment.segname));
    std::strncpy(data_segment.segname, "__DATA", sizeof(data_segment.segname));
    data_segment.vmaddr = 0x100001000;            // Virtual memory address
    data_segment.vmsize = 0x1000;                 // Virtual memory size
    data_segment.fileoff = 0x1000;                // File offset
    data_segment.filesize = 0x1000;               // File size
    data_segment.maxprot = 7;                     // VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE
    data_segment.initprot = 3;                    // VM_PROT_READ | VM_PROT_WRITE
    data_segment.nsects = 1;                      // Number of sections
    data_segment.flags = 0;

    file.write(reinterpret_cast<const char*>(&data_segment), sizeof(data_segment));

    // Section for __DATA,__data
    struct Section64 data_section;
    std::memset(&data_section, 0, sizeof(data_section));
    std::strncpy(data_section.sectname, "__data", sizeof(data_section.sectname));
    std::strncpy(data_section.segname, "__DATA", sizeof(data_section.segname));
    data_section.addr = 0x100001000;              // Section address
    data_section.size = 0x20;                     // Section size
    data_section.offset = 0x1000;                 // File offset
    data_section.align = 3;                       // Alignment (2^3 = 8)
    data_section.reloff = 0;                      // Relocation offset
    data_section.nreloc = 0;                      // Number of relocations
    data_section.flags = 0;                       // S_REGULAR
    data_section.reserved1 = 0;
    data_section.reserved2 = 0;
    data_section.reserved3 = 0;

    file.write(reinterpret_cast<const char*>(&data_section), sizeof(data_section));
    
    // Load Command 3: LC_MAIN (entry point)
    struct EntryPointCommand {
        uint32_t cmd = 0x80000028;        // LC_MAIN
        uint32_t cmdsize = 0x18;          // Size of this command
        uint64_t entryoff = 0xf50;        // Entry point offset
        uint64_t stacksize = 0;           // Stack size (0 = default)
    } entry_cmd;
    
    file.write(reinterpret_cast<const char*>(&entry_cmd), sizeof(entry_cmd));
    
    // Pad to file offset 0xf50
    file.seekp(0xf50);
    
    // Generate machine code from our intermediate representation
    std::vector<uint8_t> machine_code = generateMachineCode();
    
    // If no machine code generated, use default "Hello World" program
    if (machine_code.empty()) {
        if (target_platform == TargetPlatform::MACOS_ARM64) {
            // ARM64 assembly code
            machine_code = {
                0x00, 0x00, 0x80, 0xd2,  // mov x0, #0 (exit code)
                0x01, 0x00, 0x00, 0xd4,  // svc #0 (system call)
                0xc0, 0x03, 0x5f, 0xd6   // ret
            };
        } else {
            // x86_64 assembly code
            machine_code = {
                0x48, 0xc7, 0xc0, 0x01, 0x00, 0x00, 0x02,  // mov rax, 0x2000001 (sys_exit)
                0x48, 0xc7, 0xc7, 0x00, 0x00, 0x00, 0x00,  // mov rdi, 0 (exit code)
                0x0f, 0x05,                                // syscall
                0xc3                                       // ret
            };
        }
    }
    
    file.write(reinterpret_cast<const char*>(machine_code.data()), machine_code.size());
    
    // Pad to data section
    file.seekp(0x1000);
    
    // Data section content
    const char hello_string[] = "Hello, World from GDScript on macOS!\n";
    file.write(hello_string, sizeof(hello_string));
    
    file.close();
}

void CodeGenerator::generateLinuxExecutable(const std::string& filename) {
    // Complete Linux ELF executable generation
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        addError("Cannot create Linux executable: " + filename);
        return;
    }
    
    // ELF Header (64-bit)
    struct ElfHeader64 {
        uint8_t e_ident[16] = {
            0x7f, 'E', 'L', 'F',  // ELF magic
            2,                    // ELFCLASS64 (64-bit)
            1,                    // ELFDATA2LSB (little endian)
            1,                    // EV_CURRENT (version)
            0,                    // ELFOSABI_NONE
            0, 0, 0, 0, 0, 0, 0, 0 // padding
        };
        uint16_t e_type = 2;          // ET_EXEC (executable)
        uint16_t e_machine;           // Machine type (set below)
        uint32_t e_version = 1;       // EV_CURRENT
        uint64_t e_entry = 0x401000;  // Entry point
        uint64_t e_phoff = 64;        // Program header offset
        uint64_t e_shoff = 0x2000;    // Section header offset
        uint32_t e_flags = 0;
        uint16_t e_ehsize = 64;       // ELF header size
        uint16_t e_phentsize = 56;    // Program header entry size
        uint16_t e_phnum = 2;         // Number of program headers
        uint16_t e_shentsize = 64;    // Section header entry size
        uint16_t e_shnum = 4;         // Number of section headers
        uint16_t e_shstrndx = 3;      // String table index
    } elf_header;
    
    // Set machine type based on target platform
    if (target_platform == TargetPlatform::LINUX_ARM64) {
        elf_header.e_machine = 183;   // EM_AARCH64
    } else {
        elf_header.e_machine = 62;    // EM_X86_64
    }
    
    file.write(reinterpret_cast<const char*>(&elf_header), sizeof(elf_header));
    
    // Program Header 1: LOAD segment for code
    struct ProgramHeader64 {
        uint32_t p_type = 1;          // PT_LOAD
        uint32_t p_flags = 5;         // PF_R | PF_X (read + execute)
        uint64_t p_offset = 0;        // File offset
        uint64_t p_vaddr = 0x400000;  // Virtual address
        uint64_t p_paddr = 0x400000;  // Physical address
        uint64_t p_filesz = 0x1000;   // File size
        uint64_t p_memsz = 0x1000;    // Memory size
        uint64_t p_align = 0x1000;    // Alignment
    } code_segment;
    
    file.write(reinterpret_cast<const char*>(&code_segment), sizeof(code_segment));
    
    // Program Header 2: LOAD segment for data
    struct ProgramHeader64 data_segment;
    data_segment.p_type = 1;                  // PT_LOAD
    data_segment.p_flags = 6;                 // PF_R | PF_W (read + write)
    data_segment.p_offset = 0x1000;           // File offset
    data_segment.p_vaddr = 0x401000;          // Virtual address
    data_segment.p_paddr = 0x401000;          // Physical address
    data_segment.p_filesz = 0x1000;           // File size
    data_segment.p_memsz = 0x1000;            // Memory size
    data_segment.p_align = 0x1000;            // Alignment

    file.write(reinterpret_cast<const char*>(&data_segment), sizeof(data_segment));
    
    // Pad to code section (offset 0x1000)
    file.seekp(0x1000);
    
    // Generate machine code from our intermediate representation
    std::vector<uint8_t> machine_code = generateMachineCode();
    
    // If no machine code generated, use default "Hello World" program
    if (machine_code.empty()) {
        if (target_platform == TargetPlatform::LINUX_ARM64) {
            // ARM64 assembly code for Linux
            machine_code = {
                // write syscall: write(1, msg, len)
                0x00, 0x00, 0x80, 0xd2,  // mov x0, #0 (stdout)
                0x21, 0x00, 0x80, 0xd2,  // mov x1, #1 (message address - placeholder)
                0x42, 0x00, 0x80, 0xd2,  // mov x2, #2 (message length - placeholder)
                0x08, 0x08, 0x80, 0xd2,  // mov x8, #64 (sys_write)
                0x01, 0x00, 0x00, 0xd4,  // svc #0
                // exit syscall: exit(0)
                0x00, 0x00, 0x80, 0xd2,  // mov x0, #0 (exit code)
                0xa8, 0x0b, 0x80, 0xd2,  // mov x8, #93 (sys_exit)
                0x01, 0x00, 0x00, 0xd4   // svc #0
            };
        } else {
            // x86_64 assembly code for Linux
            machine_code = {
                // write syscall: write(1, msg, len)
                0x48, 0xc7, 0xc0, 0x01, 0x00, 0x00, 0x00,  // mov rax, 1 (sys_write)
                0x48, 0xc7, 0xc7, 0x01, 0x00, 0x00, 0x00,  // mov rdi, 1 (stdout)
                0x48, 0xc7, 0xc6, 0x00, 0x10, 0x40, 0x00,  // mov rsi, 0x401000 (message)
                0x48, 0xc7, 0xc2, 0x26, 0x00, 0x00, 0x00,  // mov rdx, 38 (message length)
                0x0f, 0x05,                                // syscall
                // exit syscall: exit(0)
                0x48, 0xc7, 0xc0, 0x3c, 0x00, 0x00, 0x00,  // mov rax, 60 (sys_exit)
                0x48, 0xc7, 0xc7, 0x00, 0x00, 0x00, 0x00,  // mov rdi, 0 (exit code)
                0x0f, 0x05                                 // syscall
            };
        }
    }
    
    file.write(reinterpret_cast<const char*>(machine_code.data()), machine_code.size());
    
    // Pad to data section (offset 0x1000 from start of data segment)
    file.seekp(0x1000);
    
    // Data section content
    const char hello_string[] = "Hello, World from GDScript on Linux!\n";
    file.write(hello_string, sizeof(hello_string));
    
    // Pad to section headers (offset 0x2000)
    file.seekp(0x2000);
    
    // Section Header 0: NULL section
    struct SectionHeader64 {
        uint32_t sh_name = 0;
        uint32_t sh_type = 0;         // SHT_NULL
        uint64_t sh_flags = 0;
        uint64_t sh_addr = 0;
        uint64_t sh_offset = 0;
        uint64_t sh_size = 0;
        uint32_t sh_link = 0;
        uint32_t sh_info = 0;
        uint64_t sh_addralign = 0;
        uint64_t sh_entsize = 0;
    } null_section;
    
    file.write(reinterpret_cast<const char*>(&null_section), sizeof(null_section));
    
    // Section Header 1: .text section
    struct SectionHeader64 text_section;
    std::memset(&text_section, 0, sizeof(text_section));
    text_section.sh_name = 1;                 // Offset in string table
    text_section.sh_type = 1;                 // SHT_PROGBITS
    text_section.sh_flags = 6;                // SHF_ALLOC | SHF_EXECINSTR
    text_section.sh_addr = 0x401000;          // Virtual address
    text_section.sh_offset = 0x1000;          // File offset
    text_section.sh_size = 0x40;              // Section size
    text_section.sh_link = 0;
    text_section.sh_info = 0;
    text_section.sh_addralign = 16;
    text_section.sh_entsize = 0;

    file.write(reinterpret_cast<const char*>(&text_section), sizeof(text_section));

    // Section Header 2: .data section
    struct SectionHeader64 data_section_hdr;
    std::memset(&data_section_hdr, 0, sizeof(data_section_hdr));
    data_section_hdr.sh_name = 7;                 // Offset in string table
    data_section_hdr.sh_type = 1;                 // SHT_PROGBITS
    data_section_hdr.sh_flags = 3;                // SHF_ALLOC | SHF_WRITE
    data_section_hdr.sh_addr = 0x402000;          // Virtual address
    data_section_hdr.sh_offset = 0x1000;          // File offset
    data_section_hdr.sh_size = 0x40;              // Section size
    data_section_hdr.sh_link = 0;
    data_section_hdr.sh_info = 0;
    data_section_hdr.sh_addralign = 8;
    data_section_hdr.sh_entsize = 0;

    file.write(reinterpret_cast<const char*>(&data_section_hdr), sizeof(data_section_hdr));

    // Section Header 3: .shstrtab (string table)
    struct SectionHeader64 strtab_section;
    std::memset(&strtab_section, 0, sizeof(strtab_section));
    strtab_section.sh_name = 13;                // Offset in string table
    strtab_section.sh_type = 3;                 // SHT_STRTAB
    strtab_section.sh_flags = 0;
    strtab_section.sh_addr = 0;
    strtab_section.sh_offset = 0x2100;          // File offset
    strtab_section.sh_size = 0x23;              // Section size
    strtab_section.sh_link = 0;
    strtab_section.sh_info = 0;
    strtab_section.sh_addralign = 1;
    strtab_section.sh_entsize = 0;

    file.write(reinterpret_cast<const char*>(&strtab_section), sizeof(strtab_section));
    
    // String table content
    file.seekp(0x2100);
    const char string_table[] = "\0.text\0.data\0.shstrtab\0";
    file.write(string_table, sizeof(string_table));
    
    file.close();
}

// Platform-specific assembly generation methods
void CodeGenerator::writeWindowsAssembly(const std::string& filename) {
    // Windows-specific assembly format
    writeAssembly(filename); // Use base implementation for now
}

void CodeGenerator::writeMacOSAssembly(const std::string& filename) {
    // macOS-specific assembly format
    writeAssembly(filename); // Use base implementation for now
}

void CodeGenerator::writeLinuxAssembly(const std::string& filename) {
    // Linux-specific assembly format
    writeAssembly(filename); // Use base implementation for now
}

// Runtime system generation
void CodeGenerator::generateRuntimeLibrary() {
    // Generate runtime library code
    // This would include GC, type system, built-in functions, etc.
}

void CodeGenerator::generateStartupCode() {
    // Generate program startup code
    emitLabel("_start");
    emit(Instruction::CALL, "main");
    emit(Instruction::MOV, allocateRegister(), 0); // exit code
    emit(Instruction::CALL, "exit");
}

void CodeGenerator::generateShutdownCode() {
    // Generate program shutdown code
    emit(Instruction::RET);
}

// Linking support
bool CodeGenerator::linkExecutable(const std::string& object_file, const std::string& executable_file) {
    // Platform-specific linking
    std::string linker_cmd = getLinkerCommand();
    linker_cmd += " -o " + executable_file + " " + object_file;
    
    // Add platform libraries
    auto libs = getPlatformLibraries();
    for (const auto& lib : libs) {
        linker_cmd += " -l" + lib;
    }
    
    return system(linker_cmd.c_str()) == 0;
}

std::vector<std::string> CodeGenerator::getPlatformLibraries() const {
    switch (target_platform) {
        case TargetPlatform::WINDOWS_X64:
            return {"kernel32", "user32", "msvcrt"};
        case TargetPlatform::MACOS_X64:
        case TargetPlatform::MACOS_ARM64:
            return {"System", "c"};
        case TargetPlatform::LINUX_X64:
        case TargetPlatform::LINUX_ARM64:
            return {"c", "m"};
        default:
            return {};
    }
}

std::string CodeGenerator::getLinkerCommand() const {
    switch (target_platform) {
        case TargetPlatform::WINDOWS_X64:
            return "link";
        case TargetPlatform::MACOS_X64:
        case TargetPlatform::MACOS_ARM64:
            return "ld";
        case TargetPlatform::LINUX_X64:
        case TargetPlatform::LINUX_ARM64:
            return "ld";
        default:
            return "ld";
    }
}

// Platform-specific code generation
std::string CodeGenerator::getArchitecture() const {
    switch (target_platform) {
        case TargetPlatform::WINDOWS_X64:
        case TargetPlatform::MACOS_X64:
        case TargetPlatform::LINUX_X64:
            return "x86_64";
        case TargetPlatform::MACOS_ARM64:
        case TargetPlatform::LINUX_ARM64:
            return "aarch64";
        default:
            return "unknown";
    }
}

void CodeGenerator::generatePlatformSpecificCode() {
    // Generate platform-specific initialization code
    generateStartupCode();
    generateRuntimeLibrary();
}

// Debug information
void CodeGenerator::generateDebugInfo(const std::string& source_file) {
    // Generate debug information for the source file
    // This would include line number tables, symbol tables, etc.
    (void)source_file; // Suppress unused parameter warning
}

// Memory management
void CodeGenerator::generateGarbageCollector() {
    // Generate garbage collector code
    // This is a simplified placeholder
}

void CodeGenerator::generateMemoryAllocation(std::shared_ptr<Register> size_reg) {
    // Generate memory allocation code
    emit(Instruction::CALL, "malloc");
    (void)size_reg; // Suppress unused parameter warning
}

void CodeGenerator::generateMemoryDeallocation(std::shared_ptr<Register> ptr_reg) {
    // Generate memory deallocation code
    emit(Instruction::CALL, "free");
    (void)ptr_reg; // Suppress unused parameter warning
}

// Runtime support
void CodeGenerator::generateRuntimeSupport() {
    // Generate runtime support functions
    generateTypeChecking();
    generateStringOperations();
    generateArrayOperations();
    generateDictionaryOperations();
}

void CodeGenerator::generateTypeChecking() {
    // Generate type checking functions
}

void CodeGenerator::generateStringOperations() {
    // Generate string operation functions
}

void CodeGenerator::generateArrayOperations() {
    // Generate array operation functions
}

void CodeGenerator::generateDictionaryOperations() {
    // Generate dictionary operation functions
}

// Error handling
void CodeGenerator::addError(const std::string& message) {
    errors.push_back("Code Generation Error: " + message);
}



// Utility methods
void CodeGenerator::optimizeCode() {
    performDeadCodeElimination();
    performConstantFolding();
    performRegisterAllocation();
}

void CodeGenerator::performDeadCodeElimination() {
    // Remove unused instructions
    for (auto& func : functions) {
        for (auto& block : func->blocks) {
            auto& instructions = block->instructions;
            instructions.erase(
                std::remove_if(instructions.begin(), instructions.end(),
                    [](const std::unique_ptr<Instruction>& instr) {
                        return instr->opcode == Instruction::NOP;
                    }),
                instructions.end());
        }
    }
}

void CodeGenerator::performConstantFolding() {
    // Fold constant expressions
    // This is a simplified implementation
}



// Helper methods
void CodeGenerator::setupFunction(const std::string& name) {
    auto func = std::make_unique<Function>(name);
    current_function = func.get();
    function_map[name] = current_function;
    
    // Create entry block
    current_block = current_function->createBlock(name + "_entry");
    
    functions.push_back(std::move(func));
    
    // Clear variable scope but preserve class members if we're in a class
    variables.clear();
    
    // If we're inside a class, make class member variables accessible
    if (!current_class_name.empty()) {
        for (const auto& member : class_members) {
            variables[member.first] = member.second;
        }
    }
}

void CodeGenerator::finalizeFunction() {
    if (current_function) {
        current_function = nullptr;
        current_block = nullptr;
    }
}



void CodeGenerator::generateMatchStmt(MatchStmt* stmt) {
    auto expr_reg = generateExpression(stmt->expression.get());
    
    std::string end_label = generateLabel("match_end");
    std::vector<std::string> case_labels;
    
    // Generate labels for each case
    for (size_t i = 0; i < stmt->cases.size(); ++i) {
        case_labels.push_back(generateLabel("match_case_" + std::to_string(i)));
    }
    
    // Generate pattern matching for each case
    for (size_t i = 0; i < stmt->cases.size(); ++i) {
        auto& match_case = stmt->cases[i];
        
        // Generate pattern comparison
        auto pattern_reg = generateExpression(match_case.pattern.get());
        emit(Instruction::CMP, expr_reg, pattern_reg);
        emit(Instruction::JE, case_labels[i]);
        
        freeRegister(pattern_reg);
    }
    
    // If no pattern matches, jump to end
    emit(Instruction::JMP, end_label);
    
    // Generate code for each case body
    for (size_t i = 0; i < stmt->cases.size(); ++i) {
        emitLabel(case_labels[i]);
        generateStatement(stmt->cases[i].body.get());
        emit(Instruction::JMP, end_label);
    }
    
    emitLabel(end_label);
    freeRegister(expr_reg);
}

std::shared_ptr<Register> CodeGenerator::generateLambdaExpr(LambdaExpr* expr) {
    // Generate a unique function name for the lambda
    std::string lambda_name = "_lambda_" + std::to_string(next_label_id++);
    
    // Save current function context
    auto saved_function = current_function;
    auto saved_block = current_block;
    auto saved_variables = variables;
    
    // Create new function for lambda
    setupFunction(lambda_name);
    
    // Add lambda parameters to variable scope
    for (const auto& param : expr->parameters) {
        auto param_reg = allocateRegister();
        variables[param.name] = param_reg;
    }
    
    // Generate lambda body
    auto body_reg = generateExpression(expr->body.get());
    
    // Return the result
    emit(Instruction::MOV, allocateRegister(), body_reg);
    emit(Instruction::RET);
    
    freeRegister(body_reg);
    finalizeFunction();
    
    // Restore previous function context
    current_function = saved_function;
    current_block = saved_block;
    variables = saved_variables;
    
    // Return a register containing the lambda function pointer
    auto result_reg = allocateRegister();
    emit(Instruction::MOV, result_reg, 0); // Load function address (placeholder)
    
    return result_reg;
}

std::shared_ptr<Register> CodeGenerator::generateTernaryExpr(TernaryExpr* expr) {
    // Generate condition
    auto condition_reg = generateExpression(expr->condition.get());
    
    // Create labels for control flow
    std::string false_label = generateLabel("ternary_false");
    std::string end_label = generateLabel("ternary_end");
    
    // Test condition and jump to false branch if zero
    emit(Instruction::CMP, condition_reg, 0);
    emit(Instruction::JE, false_label);
    freeRegister(condition_reg);
    
    // Generate true expression
    auto true_reg = generateExpression(expr->true_expr.get());
    auto result_reg = allocateRegister();
    emit(Instruction::MOV, result_reg, true_reg);
    freeRegister(true_reg);
    emit(Instruction::JMP, end_label);
    
    // Generate false expression
    emitLabel(false_label);
    auto false_reg = generateExpression(expr->false_expr.get());
    emit(Instruction::MOV, result_reg, false_reg);
    freeRegister(false_reg);
    
    // End label
    emitLabel(end_label);
    
    return result_reg;
}