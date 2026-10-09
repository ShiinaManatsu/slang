const fs = require('node:fs');
const path = require('node:path');
const assert = require('node:assert/strict');
const crypto = require('node:crypto');
const { spawn } = require('node:child_process');
const { pathToFileURL } = require('node:url');

const root = path.resolve(__dirname, '..');
const fixtures = path.join(root, 'tests/language-server');
const executable = process.argv[2];
const writeBaseline = process.argv.includes('--write-baseline');
if (!executable) throw new Error('Usage: node extras/test-hlsl-completion-profile.js <slangd> [--write-baseline]');

class Client {
    constructor() {
        this.child = spawn(executable, [], { cwd: root, windowsHide: true });
        this.pending = new Map();
        this.sequence = 0;
        this.buffer = Buffer.alloc(0);
        this.child.stderr.on('data', data => process.stderr.write(data));
        this.child.stdout.on('data', data => this.receive(data));
        this.child.on('error', error => {
            for (const reject of this.failures.values()) reject(error);
        });
        this.failures = new Map();
    }
    send(message) {
        const body = JSON.stringify({ jsonrpc: '2.0', ...message });
        this.child.stdin.write(`Content-Length: ${Buffer.byteLength(body)}\r\n\r\n${body}`);
    }
    request(method, params) {
        return new Promise((resolve, reject) => {
            const id = ++this.sequence;
            const timer = setTimeout(() => {
                this.pending.delete(id);
                this.failures.delete(id);
                reject(new Error(`Timeout: ${method}`));
            }, 30000);
            this.failures.set(id, reject);
            this.pending.set(id, message => {
                clearTimeout(timer);
                this.failures.delete(id);
                if (message.error) reject(new Error(JSON.stringify(message.error)));
                else resolve(message.result);
            });
            this.send({ id, method, params });
        });
    }
    receive(data) {
        this.buffer = Buffer.concat([this.buffer, data]);
        while (true) {
            const headerEnd = this.buffer.indexOf('\r\n\r\n');
            if (headerEnd < 0) return;
            const length = Number(/Content-Length:\s*(\d+)/i.exec(this.buffer.toString('utf8', 0, headerEnd))[1]);
            if (this.buffer.length < headerEnd + 4 + length) return;
            const message = JSON.parse(this.buffer.toString('utf8', headerEnd + 4, headerEnd + 4 + length));
            this.buffer = this.buffer.subarray(headerEnd + 4 + length);
            if (message.method === 'workspace/configuration') {
                this.send({ id: message.id, result: message.params.items.map(item => ({
                    'slang.additionalSearchPaths': [fixtures, path.join(fixtures, 'hlsl-completion-headers')],
                    'slang.mslProperties': ['UseColor'],
                    'slang.predefinedMacros': [],
                    'slang.searchInAllWorkspaceDirectories': false,
                    'slang.workspaceFlavor': 'standard',
                    'slang.enableCommitCharactersInAutoCompletion': false,
                })[item.section] ?? null) });
            } else if (message.id !== undefined && message.method) {
                this.send({ id: message.id, result: null });
            } else if (this.pending.has(message.id)) {
                this.pending.get(message.id)(message);
                this.pending.delete(message.id);
            }
        }
    }
    async initialize() {
        await this.request('initialize', { processId: process.pid, rootUri: pathToFileURL(fixtures).href, capabilities: { workspace: { configuration: true } } });
        this.send({ method: 'initialized', params: {} });
    }
    async complete(source, fileName, marker) {
        const offset = source.indexOf(`/*COMPLETE:${marker}*/`);
        assert.notEqual(offset, -1, marker);
        const prefix = source.slice(0, offset).replace(/\/\*COMPLETE:[\w-]+\*\//g, '');
        const text = source.replace(/\/\*COMPLETE:[\w-]+\*\//g, '');
        const uri = pathToFileURL(path.join(fixtures, fileName)).href;
        this.send({ method: 'textDocument/didOpen', params: { textDocument: { uri, languageId: path.extname(fileName) === '.fx' ? 'messiah-sl' : path.extname(fileName).slice(1), version: 1, text } } });
        const position = { line: prefix.split('\n').length - 1, character: prefix.length - prefix.lastIndexOf('\n') - 1 };
        const response = await this.request('textDocument/completion', { textDocument: { uri }, position, context: { triggerKind: 1 } });
        const items = Array.isArray(response) ? response : response?.items ?? [];
        for (const item of items.filter(candidate => ['includedFunction', 'add', 'Sample', 'Load', 'Store', 'GetDimensions'].includes(candidate.label))) {
            const resolved = await this.request('completionItem/resolve', item);
            assert.equal(resolved.label, item.label);
            assert.ok(resolved.detail?.includes(item.label), `${marker}: invalid resolved signature`);
        }
        this.send({ method: 'textDocument/didClose', params: { textDocument: { uri } } });
        return [...new Set(items.map(item => item.label.replace(/^COM\("[^"]+"\)$/, 'COM("<generated-guid>")')))].sort();
    }
}

const forbidden = new Set(['func', 'extension', 'import', 'typealias', 'associatedtype', 'each', 'expand', 'where', 'defer', 'throws', 'try', 'throw', 'catch', 'ref', 'nullptr', 'none', 'sizeof', 'alignof', 'IArithmetic', 'IDifferentiable', 'DifferentialPair', 'Differential', 'dzero', 'dadd', 'This', 'Differentiable', 'ForwardDifferentiable', 'BackwardDifferentiable', 'COM']);
const required = {
    'global-after': ['IncludedPayload', 'IncludedVector', 'includedFunction', 'includedColor', 'INCLUDED_MACRO', 'float4', 'sin', 'mul', 'saturate', 'ResourceDescriptorHeap', 'SamplerDescriptorHeap', 'precise', 'row_major', 'column_major', 'nointerpolation', 'snorm', 'unorm', 'linear', 'centroid', 'globallycoherent'],
    body: ['value'],
    vector: ['x', 'y', 'z', 'w'],
    scalar: ['x'],
    matrix: ['_m00', '_m22', '_11', '_33'],
    'user-member': ['add', 'color', 'weight'],
    texture: ['Sample', 'SampleLevel', 'Load', 'GetDimensions'],
    rwtexture: ['GetDimensions'],
    structured: ['GetDimensions'],
    'byte-buffer': ['Load', 'Load2', 'Load3', 'Load4', 'GetDimensions'],
    'rwbyte-buffer': ['Load', 'Store', 'InterlockedAdd'],
    'ray-query': ['Proceed', 'TraceRayInline', 'CommittedStatus'],
    attribute: ['numthreads', 'shader'],
    semantic: ['register', 'SV_Position'],
    property: ['@_UseColor'],
};

(async () => {
    const profileHeader = fs.readFileSync(path.join(root, 'source/slang/slang-language-server-hlsl-profile.h'), 'utf8').replace(/\r\n/g, '\n');
    const profileBody = /R"profile\(\n([\s\S]*?)\n\)profile"/.exec(profileHeader)?.[1];
    assert.ok(profileBody, 'Missing generated profile');
    const provenance = JSON.parse(fs.readFileSync(path.join(root, 'source/slang/slang-language-server-hlsl-profile.json'), 'utf8'));
    assert.equal(crypto.createHash('sha256').update(profileBody).digest('hex'), provenance.sha256, 'Profile checksum mismatch');
    const client = new Client();
    const snapshot = {};
    try {
        await client.initialize();
        const source = fs.readFileSync(path.join(fixtures, 'hlsl-completion-profile.hlsl'), 'utf8');
        const markers = [...source.matchAll(/\/\*COMPLETE:([\w-]+)\*\//g)].map(match => match[1]);
        for (const extension of ['hlsl', 'fx', 'hlsli', 'fxh', 'fxt', 'slang']) {
            for (const marker of markers) {
                const labels = await client.complete(source, `completion-profile-test.${extension}`, marker);
                const key = `${extension}/${marker}`;
                assert.ok(labels.length, `${key}: empty completion`);
                if (extension !== 'slang') {
                    assert.deepEqual(labels.filter(label => forbidden.has(label) || label.startsWith('SLANG_') || (label.startsWith('__') && label !== '__HLSL_VERSION')), [], `${key}: non-HLSL candidates`);
                    for (const label of (marker === 'body' ? ['vectorValue', 'scalarValue', 'payload', 'includedFunction'] : required[marker] ?? [])) assert.ok(labels.includes(label), `${key}: missing ${label}`);
                    if (marker === 'vector') assert.deepEqual(labels, [...required.vector].sort());
                    if (marker === 'scalar') assert.deepEqual(labels, required.scalar);
                } else if (marker === 'vector') {
                    assert.ok(labels.includes('add') && labels.includes('Differential'), 'Slang vector extensions must remain visible');
                }
                snapshot[key] = labels;
            }
        }
        const resources = fs.readFileSync(path.join(fixtures, 'hlsl-completion-resources.hlsl'), 'utf8');
        const resourceMarkers = [...resources.matchAll(/\/\*COMPLETE:([\w-]+)\*\//g)].map(match => match[1]);
        for (const marker of resourceMarkers) {
            const labels = await client.complete(resources, 'completion-resources-test.hlsl', marker);
            const key = `resources/${marker}`;
            assert.deepEqual(labels.filter(label => forbidden.has(label) || label.startsWith('__')), [], key);
            if (['sampler', 'comparison-sampler', 'input-patch', 'output-patch'].includes(marker)) {
                assert.deepEqual(labels, [], key);
            } else if (['constant-buffer', 'constant-buffer-method', 'buffer-element'].includes(marker)) {
                assert.deepEqual(labels, [...required['user-member']].sort(), key);
            } else if (marker === 'ray-desc') {
                assert.deepEqual(labels, ['Direction', 'Origin', 'TMax', 'TMin'], key);
            } else if (marker === 'append') {
                assert.deepEqual(labels, ['Append', 'GetDimensions'], key);
            } else if (marker === 'consume') {
                assert.deepEqual(labels, ['Consume', 'GetDimensions'], key);
            } else if (marker.endsWith('stream')) {
                assert.deepEqual(labels, ['Append', 'RestartStrip'], key);
            } else {
                assert.ok(labels.length, `${key}: empty completion`);
            }
            snapshot[key] = labels;
        }
        const property = fs.readFileSync(path.join(fixtures, 'hlsl-completion-property.fx'), 'utf8');
        for (const marker of ['property', 'body']) {
            const labels = await client.complete(property, 'completion-property-test.fx', marker);
            for (const label of required[marker]) assert.ok(labels.includes(label), `property/${marker}: missing ${label}`);
            assert.deepEqual(labels.filter(label => forbidden.has(label) || (label.startsWith('__') && label !== '__HLSL_VERSION')), []);
            snapshot[`property/${marker}`] = labels;
        }
        const fileCompletionCases = [
            { marker: 'include', source: '#include "hlsl-completion-headers//*COMPLETE:include*/"\n', expected: ['common.hlsli'] },
            { marker: 'import', source: 'import /*COMPLETE:import*/;\n', expected: [] },
            { marker: 'comment', source: '// /*COMPLETE:comment*/\nvoid shade() {}\n', expected: [] },
        ];
        for (const sample of fileCompletionCases) {
            const labels = await client.complete(sample.source, `completion-${sample.marker}-test.hlsl`, sample.marker);
            if (sample.marker === 'include') {
                assert.deepEqual(labels, sample.expected, 'include candidates');
                snapshot['files/include'] = labels;
            } else {
                assert.deepEqual(labels, sample.expected, sample.marker);
                snapshot[`files/${sample.marker}`] = labels;
            }
        }
        const baselinePath = path.join(fixtures, 'hlsl-completion-profile.expected.json');
        const candidateSets = {};
        const contexts = {};
        const setNames = new Map();
        for (const [key, labels] of Object.entries(snapshot)) {
            const serialized = JSON.stringify(labels);
            if (!setNames.has(serialized)) {
                setNames.set(serialized, key);
                candidateSets[key] = labels;
            }
            contexts[key] = setNames.get(serialized);
        }
        const baseline = { contexts, candidateSets };
        if (writeBaseline) fs.writeFileSync(baselinePath, JSON.stringify(baseline, null, 2) + '\n');
        else assert.deepEqual(baseline, JSON.parse(fs.readFileSync(baselinePath, 'utf8')), 'Completion candidate snapshot changed');
        console.log(`${Object.keys(snapshot).length} completion contexts passed`);
    } finally {
        client.child.kill();
    }
})().catch(error => {
    console.error(error);
    process.exitCode = 1;
});
