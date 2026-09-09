//TEST:LANG_SERVER(filecheck=CHECK):

@_EngineToggle
{
    Default = true;
}

void usesEngineToggle()
{
    if (@_EngineToggle)
    {
    }
}

void usesUnknownToggle()
{
    if (@_UnknownToggle)
    {
    }
}

// The synthetic property decl created for `@_UnknownToggle` must not surface in
// document symbols: as a top-level single-line sibling it breaks VS Code's
// sticky-scroll outline model (binary search over top-level start lines skips
// the enclosing function).

// DOCUMENT_SYMBOLS
// CHECK: @_EngineToggle
// CHECK: usesEngineToggle
// CHECK: usesUnknownToggle
// CHECK-NOT: @_UnknownToggle
