export const meta = {
  name: 'omission-scan',
  description: 'Read-only completeness scan for /omission-sweep: catalog sweep per chunk of inventory rows and per folder, every candidate judged by a library lens and a source lens',
  whenToUse: 'Only through /omission-sweep, which writes the scope files this script reads.',
  phases: [
    { title: 'Research', detail: 'one researcher per chunk of rows, one family sweep per folder swept whole' },
    { title: 'Verify', detail: 'library lens and source lens per candidate list; both must adopt' },
  ],
}

const CANDIDATES = {
  type: 'object', required: ['candidates', 'fetched', 'failed'],
  properties: {
    candidates: { type: 'array', items: { type: 'object', required: ['id', 'header', 'operation', 'purpose', 'sources', 'kind', 'recorded'],
      properties: {
        id: { type: 'string', description: 'stem:n, unique within this list' },
        header: { type: 'string', description: 'existing inventory path, or "new: <folder>/NN-stem.hpp" (or _NN_stem.py) for a proposed row' },
        operation: { type: 'string', description: 'usual name in the library naming style, with a parenthesised qualifier only when it changes the algorithm' },
        purpose: { type: 'string', description: 'one line, with the complexity when a source states it' },
        sources: { type: 'array', items: { type: 'string' }, description: 'URLs actually fetched that show the operation' },
        kind: { type: 'string', enum: ['missing', 'new-row', 'misplaced', 'vague'] },
        recorded: { type: 'string', description: 'the recorded omission this revisits (file and quote) and why that reason fails, or empty' },
      } } },
    fetched: { type: 'array', items: { type: 'string' } },
    failed: { type: 'array', items: { type: 'string' } },
  },
}
const VERDICTS = {
  type: 'object', required: ['verdicts'],
  properties: { verdicts: { type: 'array', items: { type: 'object', required: ['id', 'decision', 'reason', 'owner'],
    properties: {
      id: { type: 'string' },
      decision: { type: 'string', enum: ['adopt', 'omit', 'ask'] },
      reason: { type: 'string', description: 'at most 30 words; for omit, the one-line reason the folder notes will record' },
      owner: { type: 'string', description: 'inventory path that should hold it (may differ from the proposal), "new" for a new row, or empty for omit' },
    } } } },
}

const dir = args.dir
const items = [
  ...Array.from({ length: args.chunks }, (_, i) => ({ kind: 'rows', file: `${dir}/chunk-${String(i).padStart(3, '0')}.md`, label: `chunk ${i}` })),
  ...args.folders.map(f => ({ kind: 'families', folder: f, file: `${dir}/families.md`, label: `families ${f}` })),
]

const COMMON = `Read-only completeness scan of the CompProg Library; edit nothing. ` +
  `First read the folder's 00-index.md (Scope, Notes), Docs/00-notes.md (ownership and recorded omissions) and the "Not adopted" list in Docs/00-sources.md. ` +
  `Skip aliases under 00-Guidelines/03-cpp.md, One API per operation, and one-problem tricks; *mini.hpp rows and 08-Python modules are documented reductions of their full or C++ counterpart. ` +
  `Revisit a recorded omission only with a concrete reason it fails, quoted in "recorded". List the URLs you fetched and the failed fetches; invent none (Library Checker problems: github.com/yosupo06/library-checker-problems).`

const research = item => item.kind === 'rows'
  ? agent(`${COMMON}\n\nRead ${item.file} (rows with tier, owner, status, operations). For each row run the completeness sweep of 00-Guidelines/09-sources.md and report what the catalogs offer that the row lacks: ` +
      `"missing" belongs in the row, "misplaced" in another existing row (its path in "header"), "vague" names concrete operations for row wording that names none.`,
      { schema: CANDIDATES, agentType: 'researcher', effort: 'high', phase: 'Research', label: item.label })
  : agent(`${COMMON}\n\nFamily sweep for ${item.folder}; ${item.file} lists its rows. ` +
      `Sweep the category indexes of every catalog in 00-Guidelines/09-sources.md, including the research, textbook and paper sources it lists, for families in this folder's scope that no */00-index.md row covers, whether or not any library implements them. ` +
      `Report each as kind "new-row", header "new: ${item.folder}/NN-stem.hpp" (_NN_stem.py for 08-Python, 00-Guidelines/02-structure.md naming), its operations in row format in "operation", and Basic, Advanced or Esoteric first in "purpose".`,
      { schema: CANDIDATES, agentType: 'researcher', effort: 'high', phase: 'Research', label: item.label })

const LENSES = [
  ['library', `Library lens: omit what a row or header already provides in any form, what a recorded omission still excludes, and wrappers or second implementations (00-Guidelines/03-cpp.md, One API per operation); ` +
    `when Docs/00-notes.md gives it to another row, owner is that row. Otherwise adopt, owner the row that should hold it or "new".`],
  ['source', `Source lens: fetch the cited sources. Adopt what a Library Checker or CSES problem needs or two independent catalogs implement. ` +
    `A research family known only from papers or textbooks is "ask", proposed as an Esoteric row with the paper cited. Omit what the sources do not show and one-problem tricks.`],
]

const verify = (found, item) => {
  if (!found) return null
  const list = found.candidates || []
  if (!list.length) return { item: item.label, candidates: [], fetched: found.fetched || [], failed: found.failed || [] }
  const text = JSON.stringify(list, null, 1)
  return parallel(LENSES.map(([name, lens]) => () => agent(
    `${COMMON}\n\n${lens} Undecided is "ask"; one verdict per candidate id.\n\nCandidates from ${item.label} (${item.file}):\n${text}`,
    { schema: VERDICTS, agentType: 'researcher', effort: 'high', phase: 'Verify', label: `${name} ${item.label}` })))
    .then(([lib, src]) => {
      const pick = (v, id) => ((v && v.verdicts) || []).find(x => x.id === id)
      return {
        item: item.label, fetched: found.fetched, failed: found.failed,
        candidates: list.map(c => {
          const a = pick(lib, c.id), b = pick(src, c.id)
          const decision = a && b && a.decision === b.decision ? a.decision : 'ask'
          return { ...c, decision, owner: (a && a.owner) || (b && b.owner) || c.header,
            library: a ? `${a.decision}: ${a.reason}` : 'no verdict', source: b ? `${b.decision}: ${b.reason}` : 'no verdict' }
        }),
      }
    })
}

phase('Research')
log(`${items.length} research items from ${dir}`)
const results = (await pipeline(items, research, verify)).filter(Boolean)
const all = results.flatMap(r => r.candidates)
const count = d => all.filter(c => c.decision === d).length
const lost = items.length - results.length
if (lost) log(`${lost} items returned nothing (agent failure or skip); rerun their chunks`)
log(`${all.length} candidates: ${count('adopt')} adopt, ${count('omit')} omit, ${count('ask')} ask`)
return { results, missingItems: items.filter(i => !results.some(r => r.item === i.label)).map(i => i.label) }
