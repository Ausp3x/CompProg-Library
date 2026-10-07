export const meta = {
  name: 'reaudit-review',
  description: 'Read-only parallel review of verified/audit packages: one reviewer per header, findings adversarially refuted, confirmed defects returned',
  whenToUse: 'Before re-auditing P001–P017 (or any list of packages), to get a confirmed defect list without editing anything. Usage: /reaudit-review or pass package ids as args.',
  phases: [
    { title: 'Scope', detail: 'resolve packages to headers via plan.py' },
    { title: 'Review', detail: 'one read-only reviewer per header' },
    { title: 'Refute', detail: 'three skeptics per finding; majority must fail to refute' },
  ],
}

const HEADERS = {
  type: 'object', required: ['headers'],
  properties: { headers: { type: 'array', items: { type: 'object', required: ['package', 'path'],
    properties: { package: { type: 'string' }, path: { type: 'string' } } } } },
}
const FINDINGS = {
  type: 'object', required: ['findings'],
  properties: { findings: { type: 'array', items: { type: 'object', required: ['title', 'file', 'line', 'kind', 'detail', 'repro'],
    properties: { title: { type: 'string' }, file: { type: 'string' }, line: { type: 'integer' },
      kind: { type: 'string', enum: ['correctness', 'completeness', 'test-gap', 'style', 'portability'] },
      detail: { type: 'string' }, repro: { type: 'string' } } } } },
}
const VERDICT = { type: 'object', required: ['refuted', 'reason'], properties: { refuted: { type: 'boolean' }, reason: { type: 'string' } } }

phase('Scope')
const ids = Array.isArray(args) && args.length ? args : null
const scope = await agent(
  `List the library headers to review. ${ids ? `Packages: ${ids.join(', ')}.` : 'Use every package whose status is "audit".'} ` +
  `Run python3 '00-Guidelines/13-Plan/plan.py' status and show <id> for each package; return each target path that exists on disk with its package id. ` +
  `Skip support packages with no targets.`,
  { schema: HEADERS, effort: 'low', label: 'scope' })
const headers = scope ? scope.headers : []
log(`${headers.length} headers to review`)

const results = await pipeline(
  headers,
  h => agent(
    `Review ${h.path} (package ${h.package}) of the CompProg Library as an independent reviewer. ` +
    `Run python3 '00-Guidelines/13-Plan/plan.py' show ${h.package} for the inventory row and evidence. ` +
    `Read the header, its evidence document and its tester. Report only defects you can support: correctness bugs with a concrete failing input, ` +
    `operations named in the inventory row but absent, inventory operations with no test, violations of 00-Guidelines/03-cpp.md (closing-brace rule, std:: list, ` +
    `complexity comments, use of long, POSIX-only calls), and judge-portability problems. Do not edit files. Return an empty list if nothing is wrong.`,
    { schema: FINDINGS, agentType: 'reviewer', phase: 'Review', label: h.path }),
  (r, h) => parallel((r ? r.findings : []).map(f => () =>
    parallel([0, 1, 2].map(i => () => agent(
      `Skeptic ${i + 1}: try to refute this review finding about ${f.file}:${f.line} in the CompProg Library.\n` +
      `Title: ${f.title}\nKind: ${f.kind}\nDetail: ${f.detail}\nClaimed repro: ${f.repro}\n` +
      `Read the file and, for correctness claims, compile and run the repro under /tmp. Default to refuted=true if you cannot confirm it. Do not edit files.`,
      { schema: VERDICT, phase: 'Refute', label: `${f.file}:${f.line}` })))
      .then(vs => ({ ...f, package: h.package, votes: vs.filter(Boolean), confirmed: vs.filter(Boolean).filter(v => !v.refuted).length >= 2 })))),
)

const all = results.filter(Boolean).flat()
const confirmed = all.filter(f => f.confirmed)
log(`${confirmed.length} confirmed of ${all.length} findings`)
return { confirmed, rejected: all.filter(f => !f.confirmed).map(f => ({ title: f.title, file: f.file, line: f.line })) }
