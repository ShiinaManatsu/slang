#include "slang-language-server-document-symbols.h"
#include "slang-ast-print.h"

namespace Slang
{
struct GetDocumentSymbolContext
{
    HashSet<Decl*> processedDecls;
    DocumentVersion* doc;
    Linkage* linkage;
    UnownedStringSlice fileName;
};

static LanguageServerProtocol::SymbolKind _getSymbolKind(Decl* decl)
{
    // A source-level `cbuffer` is lowered to a transparent ConstantBuffer<T>
    // variable plus an implicit struct T. Expose the variable as a container
    // so document symbols follow the declaration the user actually wrote.
    if (auto varDecl = as<VarDeclBase>(decl))
    {
        if (varDecl->hasModifier<ImplicitParameterGroupVariableModifier>() &&
            as<ConstantBufferType>(varDecl->type))
        {
            return LanguageServerProtocol::kSymbolKindNamespace;
        }
    }
    if (decl->hasModifier<ImplicitParameterGroupElementTypeModifier>())
    {
        return -1;
    }
    if (as<StructDecl>(decl))
    {
        return LanguageServerProtocol::kSymbolKindStruct;
    }
    if (as<ClassDecl>(decl))
    {
        return LanguageServerProtocol::kSymbolKindClass;
    }
    if (as<InterfaceDecl>(decl))
    {
        return LanguageServerProtocol::kSymbolKindInterface;
    }
    if (as<FuncDecl>(decl))
    {
        return as<AggTypeDecl>(decl->parentDecl) ? LanguageServerProtocol::kSymbolKindMethod
                                                 : LanguageServerProtocol::kSymbolKindFunction;
    }
    if (as<PropertyDecl>(decl))
    {
        return LanguageServerProtocol::kSymbolKindProperty;
    }
    if (as<ConstructorDecl>(decl))
    {
        return LanguageServerProtocol::kSymbolKindConstructor;
    }
    if (as<AssocTypeDecl>(decl))
    {
        return LanguageServerProtocol::kSymbolKindTypeParameter;
    }
    if (as<VarDeclBase>(decl))
    {
        if (decl->findModifier<ConstModifier>())
            return LanguageServerProtocol::kSymbolKindConstant;
        return as<AggTypeDecl>(decl->parentDecl) ? LanguageServerProtocol::kSymbolKindField
                                                 : LanguageServerProtocol::kSymbolKindVariable;
    }
    if (as<TypeDefDecl>(decl))
    {
        return LanguageServerProtocol::kSymbolKindClass;
    }
    if (as<GenericTypeParamDecl>(decl))
    {
        return LanguageServerProtocol::kSymbolKindTypeParameter;
    }
    if (as<GenericValueParamDecl>(decl))
    {
        return LanguageServerProtocol::kSymbolKindConstant;
    }
    if (as<EnumDecl>(decl))
    {
        return LanguageServerProtocol::kSymbolKindEnum;
    }
    if (as<EnumCaseDecl>(decl))
    {
        return LanguageServerProtocol::kSymbolKindEnumMember;
    }
    if (decl->findModifier<MSLTechniqueModifier>())
    {
        return LanguageServerProtocol::kSymbolKindObject;
    }
    if (as<NamespaceDecl>(decl))
    {
        return LanguageServerProtocol::kSymbolKindNamespace;
    }
    if (as<ExtensionDecl>(decl))
    {
        return LanguageServerProtocol::kSymbolKindClass;
    }
    if (as<SubscriptDecl>(decl))
    {
        return LanguageServerProtocol::kSymbolKindOperator;
    }
    return -1;
}

static StructDecl* _getCBufferElementType(Decl* decl)
{
    auto varDecl = as<VarDeclBase>(decl);
    if (!varDecl || !varDecl->hasModifier<ImplicitParameterGroupVariableModifier>())
        return nullptr;

    auto cbufferType = as<ConstantBufferType>(varDecl->type);
    if (!cbufferType)
        return nullptr;

    auto elementType = as<DeclRefType>(cbufferType->getElementType());
    return elementType ? as<StructDecl>(elementType->getDeclRef().getDecl()) : nullptr;
}

static String _getDocumentSymbolDetail(Decl* decl, ASTBuilder* astBuilder)
{
    auto varDecl = as<VarDeclBase>(decl);
    if (!varDecl || !varDecl->parentDecl ||
        !varDecl->parentDecl->hasModifier<ImplicitParameterGroupElementTypeModifier>() ||
        !varDecl->type.type)
    {
        return String();
    }

    ASTPrinter printer(
        astBuilder,
        ASTPrinter::OptionFlag::NoInternalKeywords | ASTPrinter::OptionFlag::SimplifiedBuiltinType);
    printer.addType(varDecl->type.type);
    return printer.getString();
}

static SourceLoc _findClosingSourceLoc(Decl* decl)
{
    if (auto func = as<FunctionDeclBase>(decl))
    {
        if (auto block = as<BlockStmt>(func->body))
        {
            return block->closingSourceLoc;
        }
        else if (auto unparsedStmt = as<UnparsedStmt>(func->body))
        {
            if (unparsedStmt->tokens.getCount())
                return unparsedStmt->tokens.getLast().getLoc();
        }
        else if (func->body)
        {
            return func->body->loc;
        }
    }
    if (auto container = as<ContainerDecl>(decl))
    {
        return container->closingSourceLoc;
    }
    if (auto cbufferElementType = _getCBufferElementType(decl))
    {
        return cbufferElementType->closingSourceLoc;
    }
    return SourceLoc();
}

static NameLoc _getDeclRefExprNameLoc(Expr* expr)
{
    if (auto varExpr = as<VarExpr>(expr))
    {
        return NameLoc(varExpr->name, varExpr->loc);
    }
    else if (auto appBase = as<AppExprBase>(expr))
    {
        return _getDeclRefExprNameLoc(appBase->functionExpr);
    }
    return NameLoc();
}
static NameLoc _getDeclNameLoc(Decl* decl)
{
    if (auto varDecl = as<VarDeclBase>(decl))
    {
        if (varDecl->hasModifier<ImplicitParameterGroupVariableModifier>())
        {
            if (auto reflectionName = decl->findModifier<ParameterGroupReflectionName>())
                return reflectionName->nameAndLoc;
        }
    }
    if (auto extDecl = as<ExtensionDecl>(decl))
    {
        return _getDeclRefExprNameLoc(extDecl->targetType.exp);
    }
    if (auto techniqueModifier = decl->findModifier<MSLTechniqueModifier>())
    {
        if (techniqueModifier->nameToken.type == TokenType::Identifier)
            return NameLoc(techniqueModifier->nameToken);
    }
    return decl->nameAndLoc;
}

static bool _isInCurrentDocument(
    GetDocumentSymbolContext& context,
    const HumaneSourceLoc& humaneLoc)
{
    return humaneLoc.line != 0 &&
           context.fileName.endsWithCaseInsensitive(
               Path::getFileName(humaneLoc.pathInfo.foundPath).getUnownedSlice());
}

static SourceLoc _findCBufferMacroInvocationLoc(
    GetDocumentSymbolContext& context,
    Decl* cbufferDecl,
    VarDeclBase* memberDecl)
{
    auto sourceManager = context.linkage->getSourceManager();
    auto memberNameLoc = _getDeclNameLoc(memberDecl);
    if (!memberNameLoc.name || !memberNameLoc.loc.isValid())
        return SourceLoc();

    auto cbufferNameLoc = _getDeclNameLoc(cbufferDecl);
    auto cbufferStart = sourceManager->getHumaneLoc(cbufferNameLoc.loc, SourceLocType::Actual);
    auto cbufferEndLoc = _findClosingSourceLoc(cbufferDecl);
    auto cbufferEnd = sourceManager->getHumaneLoc(cbufferEndLoc, SourceLocType::Actual);
    if (!_isInCurrentDocument(context, cbufferStart) || !_isInCurrentDocument(context, cbufferEnd))
        return SourceLoc();

    auto& preprocessorInfo = context.linkage->contentAssistInfo.preprocessorInfo;
    for (auto& definition : preprocessorInfo.macroDefinitions)
    {
        if (!definition.name)
            continue;

        bool definesMember = false;
        for (auto& token : definition.tokenList)
        {
            if (token.loc == memberNameLoc.loc && token.getNameOrNull() == memberNameLoc.name)
            {
                definesMember = true;
                break;
            }
        }
        if (!definesMember)
            continue;

        for (auto& invocation : preprocessorInfo.macroInvocations)
        {
            if (invocation.name != definition.name)
                continue;

            auto invocationLoc = sourceManager->getHumaneLoc(invocation.loc, SourceLocType::Actual);
            if (!_isInCurrentDocument(context, invocationLoc))
                continue;
            if (invocationLoc.line < cbufferStart.line || invocationLoc.line > cbufferEnd.line)
                continue;

            return invocation.loc;
        }
    }
    return SourceLoc();
}

static void _getCBufferMemberSymbols(
    GetDocumentSymbolContext& context,
    Decl* cbufferDecl,
    StructDecl* cbufferElementType,
    List<LanguageServerProtocol::DocumentSymbol>& childSymbols)
{
    auto srcManager = context.linkage->getSourceManager();
    for (auto member : cbufferElementType->getDirectMemberDecls())
    {
        if (auto genericDecl = as<GenericDecl>(member))
            member = genericDecl->inner;
        if (!as<VarDeclBase>(member))
            continue;

        NameLoc nameLoc = _getDeclNameLoc(member);
        if (!nameLoc.name || nameLoc.name->text.getLength() == 0 || !nameLoc.loc.isValid())
            continue;
        if (member->hasModifier<SynthesizedModifier>() ||
            member->hasModifier<ToBeSynthesizedModifier>())
        {
            continue;
        }

        SourceLoc symbolLoc = nameLoc.loc;
        auto humaneLoc = srcManager->getHumaneLoc(symbolLoc, SourceLocType::Actual);
        if (!_isInCurrentDocument(context, humaneLoc))
        {
            symbolLoc = _findCBufferMacroInvocationLoc(context, cbufferDecl, as<VarDeclBase>(member));
            humaneLoc = srcManager->getHumaneLoc(symbolLoc, SourceLocType::Actual);
        }
        if (!_isInCurrentDocument(context, humaneLoc))
        {
            continue;
        }

        LanguageServerProtocol::DocumentSymbol symbol;
        symbol.name = nameLoc.name->text;
        symbol.kind = LanguageServerProtocol::kSymbolKindField;
        symbol.detail = _getDocumentSymbolDetail(member, context.linkage->getASTBuilder());
        Index line, col;
        context.doc->oneBasedUTF8LocToZeroBasedUTF16Loc(
            humaneLoc.line,
            humaneLoc.column,
            line,
            col);
        symbol.selectionRange.start.line = (int)line;
        symbol.selectionRange.start.character = (int)col;
        symbol.selectionRange.end.line = (int)line;
        symbol.selectionRange.end.character =
            (int)(col + (int)UTF8Util::calcUTF16CharCount(nameLoc.name->text.getUnownedSlice()));
        symbol.range.start.line = (int)line;
        symbol.range.start.character = 0;
        symbol.range.end = symbol.selectionRange.end;
        childSymbols.add(_Move(symbol));
    }
}

static void _getDocumentSymbolsImpl(
    GetDocumentSymbolContext& context,
    Decl* parent,
    List<LanguageServerProtocol::DocumentSymbol>& childSymbols)
{
    auto containerDecl = as<ContainerDecl>(parent);
    if (!containerDecl)
        return;
    if (!context.processedDecls.add(parent))
        return;
    auto srcManager = context.linkage->getSourceManager();
    for (auto child : containerDecl->getDirectMemberDecls())
    {
        if (auto genericDecl = as<GenericDecl>(child))
        {
            child = genericDecl->inner;
        }
        LanguageServerProtocol::SymbolKind kind = _getSymbolKind(child);
        if (kind <= 0)
            continue;
        NameLoc nameLoc = _getDeclNameLoc(child);
        if (!nameLoc.name)
            continue;
        if (nameLoc.name->text.getLength() == 0)
            continue;
        if (!nameLoc.loc.isValid())
            continue;
        if (child->hasModifier<SynthesizedModifier>() ||
            child->hasModifier<ToBeSynthesizedModifier>())
            continue;
        auto humaneLoc = srcManager->getHumaneLoc(nameLoc.loc, SourceLocType::Actual);
        if (humaneLoc.line == 0)
            continue;
        if (context.fileName.endsWithCaseInsensitive(
                Path::getFileName(humaneLoc.pathInfo.foundPath).getUnownedSlice()))
        {
            LanguageServerProtocol::DocumentSymbol sym;
            sym.name = nameLoc.name->text;
            sym.kind = kind;
            sym.detail = _getDocumentSymbolDetail(child, context.linkage->getASTBuilder());
            if (_getCBufferElementType(child))
                sym.detail = "cbuffer";
            Index line, col;
            context.doc
                ->oneBasedUTF8LocToZeroBasedUTF16Loc(humaneLoc.line, humaneLoc.column, line, col);
            sym.selectionRange.start.line = (int)line;
            sym.selectionRange.start.character = (int)col;
            sym.selectionRange.end.line = (int)line;
            sym.selectionRange.end.character =
                (int)(col +
                      (int)UTF8Util::calcUTF16CharCount(nameLoc.name->text.getUnownedSlice()));
            sym.range.start.line = (int)line;
            sym.range.start.character = 0;
            sym.range.end.line = (int)line;
            sym.range.end.character = sym.selectionRange.end.character;
            // Now try to find the end of the decl.
            auto closingLoc = _findClosingSourceLoc(child);
            if (closingLoc.isValid())
            {
                auto closingHumaneLoc = srcManager->getHumaneLoc(closingLoc, SourceLocType::Actual);
                context.doc->oneBasedUTF8LocToZeroBasedUTF16Loc(
                    closingHumaneLoc.line,
                    closingHumaneLoc.column,
                    line,
                    col);
                // Due to macro replacements, the closing loc may be before the start loc,
                // and we need to make sure never returning such invalid ranges to the editor
                // client.
                if (closingHumaneLoc.line > sym.range.start.line ||
                    closingHumaneLoc.line == sym.range.start.line &&
                        closingHumaneLoc.column >= sym.range.start.character)
                {
                    sym.range.end.line = (int)line;
                    sym.range.end.character = (int)col;
                }
                if (sym.selectionRange.end.line == sym.range.end.line ||
                    sym.selectionRange.end.character >= sym.range.end.character)
                {
                    sym.selectionRange.end = sym.range.end;
                }
            }
            if (auto cbufferElementType = _getCBufferElementType(child))
            {
                // The transparent cbuffer variable is an empty container. Its
                // source members live on the implicit element struct instead.
                _getCBufferMemberSymbols(context, child, cbufferElementType, sym.children);
            }
            else if (const auto childContainerDecl = as<ContainerDecl>(child); childContainerDecl)
            {
                // Recurse
                bool shouldRecurse = true;
                if (as<CallableDecl>(child))
                    shouldRecurse = false;
                if (as<PropertyDecl>(child))
                    shouldRecurse = false;
                if (shouldRecurse)
                {
                    _getDocumentSymbolsImpl(context, child, sym.children);
                }
            }
            childSymbols.add(_Move(sym));
        }
    }
}

List<LanguageServerProtocol::DocumentSymbol> getDocumentSymbols(
    Linkage* linkage,
    Module* module,
    UnownedStringSlice fileName,
    DocumentVersion* doc)
{
    GetDocumentSymbolContext context;
    context.fileName = fileName;
    context.doc = doc;
    context.linkage = linkage;
    List<LanguageServerProtocol::DocumentSymbol> result;
    _getDocumentSymbolsImpl(context, module->getModuleDecl(), result);
    return result;
}

} // namespace Slang
