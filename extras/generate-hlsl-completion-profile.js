const fs = require('node:fs');
const path = require('node:path');
const crypto = require('node:crypto');
const { execFileSync } = require('node:child_process');

const checkout = process.argv[2];
if (!checkout) throw new Error('Usage: node extras/generate-hlsl-completion-profile.js <DXC checkout>');
const read = relative => fs.readFileSync(path.join(checkout, relative), 'utf8');
const stripComments = text => text.replace(/\/\*[\s\S]*?\*\//g, '').replace(/\/\/[^\r\n]*/g, '');
const intrinsics = stripComments(read('utils/hct/gen_intrin_main.txt'));
const records = [];
for (const namespace of intrinsics.matchAll(/namespace\s+(\w+)\s*\{([\s\S]*?)\}\s*namespace/g)) {
    let owners = [namespace[1].replace(/Methods$/, '')];
    if (namespace[1] === 'Intrinsics') owners = [''];
    else if (namespace[1] === 'StreamMethods') owners = ['PointStream', 'LineStream', 'TriangleStream'];
    else if (namespace[1] === 'GroupOrThreadNodeOutputRecordsMethods') owners = ['GroupNodeOutputRecords', 'ThreadNodeOutputRecords'];
    else if (namespace[1] === 'Texture2DArrayMSMethods') owners = ['Texture2DArrayMS'];
    else if (namespace[1] === 'DxHitObjectMethods') owners = ['HitObject'];
    else if (namespace[1].startsWith('Vk') || namespace[1] === 'DxIntrinsics') continue;
    owners = owners.map(owner => owner.replace('CUBE', 'Cube').replace('MSArray', 'ArrayMS'));
    const names = [...new Set([...namespace[2].matchAll(/\[\[[^\]]*\]\]\s*(\w+)\s*\(/g)].map(match => match[1]))];
    if (!names.length) throw new Error(`No intrinsics parsed for ${namespace[1]}`);
    for (const owner of owners) {
        const expandedOwners = owner.startsWith('RW') ? [owner, owner.replace(/^RW/, 'RasterizerOrdered')] : [owner];
        for (const expandedOwner of expandedOwners) for (const name of names) records.push([expandedOwner ? 'member' : 'function', expandedOwner, name]);
    }
}
if (records.filter(record => record[0] === 'function').length < 150) throw new Error('Incomplete function table');
const sema = stripComments(read('tools/clang/lib/Sema/SemaHLSL.cpp'));
const ast = stripComments(read('tools/clang/lib/AST/ASTContextHLSL.cpp'));
const strings = text => [...text.matchAll(/"([A-Za-z_]\w*)"/g)].map(match => match[1]);
const scalarBlock = /HLSLScalarTypeNames\[\]\s*=\s*\{([\s\S]*?)\};/.exec(ast);
const typeBlock = /g_ArBasicTypeNames\[\]\s*=\s*\{([\s\S]*?)\};/.exec(sema);
if (!scalarBlock || !typeBlock) throw new Error('Missing DXC type table');
const scalarTypes = strings(scalarBlock[1]).filter(name => !['int8_t4_packed', 'uint8_t4_packed'].includes(name));
const types = new Set([...strings(typeBlock[1]), ...scalarTypes, 'vector', 'matrix']);
for (const name of ['SubpassInput', 'SubpassInputMS', 'SpirvType', 'SpirvOpaqueType', 'integral_constant', 'Literal', 'ext_type', 'ext_result_id', 'BufferPointer', 'SampledTexture1D', 'SampledTexture1DArray', 'SampledTexture2D', 'SampledTexture2DArray', 'SampledTexture2DMS', 'SampledTexture2DMSArray', 'SampledTextureCUBE', 'SampledTextureCUBEArray', 'SampledTexture3D']) types.delete(name);
for (const scalar of scalarTypes) {
    for (let rows = 1; rows <= 4; rows++) {
        types.add(`${scalar}${rows}`);
        for (let columns = 1; columns <= 4; columns++) types.add(`${scalar}${rows}x${columns}`);
    }
}
for (const name of types) records.push(['type', '', name]);
const constantBlock = ast.slice(ast.indexOf('void hlsl::AddRaytracingConstants'), ast.indexOf('static Expr *IntConstantAsBoolExpr'));
for (const name of strings(constantBlock)) records.push(['constant', '', name]);
for (const name of ['ResourceDescriptorHeap', 'SamplerDescriptorHeap']) records.push(['constant', '', name]);
const attributes = stripComments(read('tools/clang/include/clang/Basic/Attr.td'));
for (const block of attributes.matchAll(/def HLSL\w+\s*:\s*\w+\s*\{([\s\S]*?)\}/g)) {
    for (const spelling of block[1].matchAll(/CXX11<"",\s*"(\w+)"/g)) records.push(['attribute', '', spelling[1]]);
}
for (const keyword of stripComments(read('tools/clang/include/clang/Basic/TokenKinds.def')).matchAll(/KEYWORD\(\s*(\w+)\s*,[^\n)]*KEYHLSL/g)) records.push(['keyword', '', keyword[1]]);
for (const keyword of ['static', 'const', 'class', 'struct', 'typedef', 'namespace', 'using', 'enum', 'if', 'else', 'switch', 'case', 'default', 'return', 'while', 'for', 'do', 'this', 'break', 'continue', 'true', 'false']) records.push(['keyword', '', keyword]);
for (const semantic of stripComments(read('lib/DXIL/DxilSemantic.cpp')).matchAll(/"SV_(\w+)"/g)) records.push(['semantic', '', `sv_${semantic[1].toLowerCase()}`]);
for (const semantic of ['register', 'packoffset', 'read', 'write']) records.push(['semantic', '', semantic]);
for (const [owner, names] of Object.entries({
    RayDesc: ['Origin', 'Direction', 'TMin', 'TMax'],
    BuiltInTriangleIntersectionAttributes: ['barycentrics'],
    BuiltInTrianglePositions: ['vertex0', 'vertex1', 'vertex2'],
})) for (const name of names) records.push(['member', owner, name]);
const unique = [...new Map(records.map(record => [record.join('|'), record])).values()].sort((left, right) => left.join('|').localeCompare(right.join('|'), 'en'));
const body = unique.map(record => record.join('|')).join('\n');
const destination = path.resolve(__dirname, '../source/slang/slang-language-server-hlsl-profile.h');
fs.writeFileSync(destination, `#pragma once\n\nnamespace Slang\n{\nstatic const char kHlslCompletionProfile[] = R"profile(\n${body}\n)profile";\n}\n`);
const revision = execFileSync('git', ['-C', checkout, 'rev-parse', 'HEAD'], { encoding: 'utf8' }).trim();
fs.writeFileSync(path.resolve(__dirname, '../source/slang/slang-language-server-hlsl-profile.json'), JSON.stringify({ revision, sha256: crypto.createHash('sha256').update(body).digest('hex'), sources: ['utils/hct/gen_intrin_main.txt', 'tools/clang/lib/Sema/SemaHLSL.cpp', 'tools/clang/lib/AST/ASTContextHLSL.cpp', 'tools/clang/include/clang/Basic/Attr.td', 'tools/clang/include/clang/Basic/TokenKinds.def', 'lib/DXIL/DxilSemantic.cpp'] }, null, 2) + '\n');
console.log(JSON.stringify({ revision, sha256: crypto.createHash('sha256').update(body).digest('hex'), records: unique.length, destination }));
