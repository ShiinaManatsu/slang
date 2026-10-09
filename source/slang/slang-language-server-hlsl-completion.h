#pragma once

#include "slang-language-server-completion.h"

namespace Slang
{
bool isHlslCompletion(UnownedStringSlice path);
bool isHlslCompletionDecl(DeclRef<Decl> declRef, ASTBuilder* astBuilder);
bool isHlslCompletionKeyword(const char* keyword);
bool isHlslCompletionSemantic(const char* semantic);
} // namespace Slang
