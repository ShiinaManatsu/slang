# HLSL completion profile

HLSL and Messiah documents use a completion-only visibility profile. Slang documents retain the
existing candidates. Parsing, semantic lookup, diagnostics, and the standard library are unchanged.

The profile classifies builtins by declaration category and owning type. User declarations and MSL
properties do not require profile entries. Scalar/vector/matrix implementation members, synthesized
parameter-group declarations, and Slang syntax are excluded from HLSL completion.

`source/slang/slang-language-server-hlsl-profile.h` is generated from the DXC source checkout at
revision `1e4181c0f4cede851b9fa67a017717135849ba3d`. Sources are `gen_intrin_main.txt`,
`SemaHLSL.cpp`, `ASTContextHLSL.cpp`, `Attr.td`, `TokenKinds.def`, and `DxilSemantic.cpp`.
The adjacent JSON records the input revision and profile checksum. Generation does not scrape
documentation or run during ordinary builds.

```text
node extras/generate-hlsl-completion-profile.js <DXC checkout>
node extras/test-hlsl-completion-profile.js <slangd executable>
```

The regression uses fixed HLSL and MSL fixtures, included declarations, required/forbidden candidates,
and exact candidate-set snapshots. Equivalent sets share one baseline entry. The Slang dialect is
tested as an unfiltered control. `completionItem/resolve` is checked for selected retained candidates.
The random GUID in Slang's generated COM suggestion is normalized in snapshots.

The suite checks 150 contexts: 19 syntax/member positions across six file dialects, 31 additional
resource positions, two MSL property positions, and include/import/comment boundaries. Resource cases
include array/MS/cube textures, read/write buffers, append/consume buffers, rasterizer-ordered resources,
sampler feedback, ray queries, patches, geometry streams, and transparent constant-buffer members.
The main HLSL fixture also compiles with DXC as a compute shader.

```text
node extras/test-hlsl-completion-profile.js <slangd executable> --write-baseline
```

Review candidate differences before accepting a new baseline. The profile describes HLSL source
availability, not the current shader stage, shader model, or entry-point reachability. It filters
existing candidates; it does not add missing Slang implementations.
