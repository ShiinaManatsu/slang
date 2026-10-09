#include "slang-language-server-hlsl-completion.h"

#include "slang-check.h"
#include "slang-language-server-hlsl-profile.h"
#include "slang-options.h"
#include "slang-syntax.h"

namespace Slang
{
struct HlslCompletionProfile
{
    HashSet<String> items;

    HlslCompletionProfile()
    {
        auto text = UnownedStringSlice(kHlslCompletionProfile);
        while (text.getLength())
        {
            auto newline = text.indexOf('\n');
            if (newline == -1)
                break;
            auto item = text.head(newline);
            if (item.getLength())
                items.add(String(item));
            text = text.tail(newline + 1);
        }
    }
};

static bool _containsProfileItem(const char* category, String owner, UnownedStringSlice name)
{
    static const HlslCompletionProfile profile;
    StringBuilder key;
    key << category << "|" << owner << "|" << name;
    return profile.items.contains(key.produceString());
}

bool isHlslCompletion(UnownedStringSlice path)
{
    Stage stage;
    return findSourceLanguageFromPath(String(path), stage) == SLANG_SOURCE_LANGUAGE_HLSL ||
           path.endsWithCaseInsensitive(".fx") || path.endsWithCaseInsensitive(".fxh") ||
           path.endsWithCaseInsensitive(".fxt") || path.endsWithCaseInsensitive(".hlsli");
}

bool isHlslCompletionKeyword(const char* keyword)
{
    return _containsProfileItem("keyword", "", UnownedStringSlice(keyword));
}

bool isHlslCompletionSemantic(const char* semantic)
{
    String normalized = String(semantic).toLower();
    return _containsProfileItem("semantic", "", normalized.getUnownedSlice());
}

bool isHlslCompletionDecl(DeclRef<Decl> declRef, ASTBuilder* astBuilder)
{
    auto decl = declRef.getDecl();
    if (auto genericDecl = as<GenericDecl>(decl))
    {
        declRef = astBuilder->getMemberDeclRef(declRef.as<GenericDecl>(), genericDecl->inner);
        decl = declRef.getDecl();
    }
    if (!decl || !decl->getName())
        return false;
    if (as<SyntaxDecl>(decl))
        return isHlslCompletionKeyword(decl->getName()->text.getBuffer());
    if (!decl->nameAndLoc.loc.isValid() && !isFromCoreModule(decl) &&
        !decl->hasModifier<MSLPropertyModifier>())
        return false;
    if (decl->hasModifier<ImplicitParameterGroupElementTypeModifier>() ||
        decl->hasModifier<ImplicitParameterGroupVariableModifier>() ||
        decl->hasModifier<SynthesizedModifier>())
        return false;
    if (!isFromCoreModule(decl))
        return true;
    auto name = decl->getName()->text.getUnownedSlice();
    if (as<AttributeDecl>(decl))
        return _containsProfileItem("attribute", "", name);
    auto parent = getParentDeclRef(declRef);
    Type* ownerType = nullptr;
    if (auto extension = parent.as<ExtensionDecl>())
        ownerType = getTargetType(astBuilder, extension);
    else if (parent.as<AggTypeDecl>())
        ownerType = DeclRefType::create(astBuilder, parent);
    if (ownerType)
    {
        auto canonicalOwner = ownerType->getCanonicalType();
        if (as<BasicExpressionType>(canonicalOwner) || as<VectorExpressionType>(canonicalOwner) ||
            as<MatrixExpressionType>(canonicalOwner))
            return false;
        String owner = canonicalOwner->toString();
        auto genericStart = owner.indexOf('<');
        if (genericStart != -1)
            owner = owner.getUnownedSlice().head(genericStart);
        return _containsProfileItem("member", owner.getUnownedSlice(), name);
    }
    if (as<FuncDecl>(decl))
        return _containsProfileItem("function", "", name);
    if (as<VarDeclBase>(decl) || as<EnumCaseDecl>(decl))
        return _containsProfileItem("constant", "", name);
    if (as<AggTypeDecl>(decl) || as<SimpleTypeDecl>(decl))
        return _containsProfileItem("type", "", name);
    return false;
}
} // namespace Slang
