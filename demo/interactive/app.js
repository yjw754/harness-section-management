const sides = ['positive', 'negative'];
const sideNames = {positive: '正侧', negative: '负侧'};
const lookup = (name) => document.querySelector(`[data-testid="${name}"]`);
const record = (value) => value !== null && typeof value === 'object' && !Array.isArray(value);
const nonempty = (value) => typeof value === 'string' && value.length > 0;
const fieldError = (path, message) => ({path, message});
const sectionOptions = (radius) => ({shape: {kind: 'circular', radius}, rotation: 0, spacing: 0, autoResize: false});

function initialCase() {
  return {
    schemaVersion: 1,
    modelingTolerance: 1e-9,
    positionTransform: [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1],
    roots: {positive: sectionOptions(20), negative: sectionOptions(20)},
    sections: [
      {id: 'p-main', label: 'P main', side: 'positive', parentId: null, ...sectionOptions(8)},
      {id: 'n-main', label: 'N main', side: 'negative', parentId: null, ...sectionOptions(8)},
    ],
    relations: [{positiveId: 'p-main', negativeId: 'n-main', samePosition: true}],
  };
}

// Follow the native rigid-transform validation, including the fixed tolerance.
function isRigid(matrix) {
  if (!Array.isArray(matrix) || matrix.length !== 16 || !matrix.every(Number.isFinite)) return false;
  const near = (a, b) => Math.abs(a - b) <= 1e-9;
  for (let i = 0; i < 4; i += 1) if (!near(matrix[12 + i], i === 3 ? 1 : 0)) return false;
  for (let i = 0; i < 3; i += 1) {
    for (let j = 0; j < 3; j += 1) {
      const dot = matrix[i] * matrix[j] + matrix[4 + i] * matrix[4 + j] + matrix[8 + i] * matrix[8 + j];
      if (!near(dot, i === j ? 1 : 0)) return false;
    }
  }
  const [a, b, c, , d, e, f, , g, h, i] = matrix;
  return near(a * (e * i - f * h) - b * (d * i - f * g) + c * (d * h - e * g), 1);
}

export function validateCaseDraft(draft) {
  const errors = [];
  const add = (path, message) => errors.push(fieldError(path, message));
  if (!record(draft)) return [fieldError('$', '案例必须为对象')];
  if (draft.schemaVersion !== 1) add('schemaVersion', '仅支持版本 1');
  const tolerance = draft.modelingTolerance;
  if (tolerance !== 1e-9) add('modelingTolerance', '当前圆形求解器的建模公差仅支持 1e-9');
  if (!isRigid(draft.positionTransform)) add('positionTransform', '必须为包含 16 个有限数字的刚体变换矩阵');

  const validateOptions = (value, path) => {
    if (!record(value)) { add(path, '必须为对象'); return; }
    if (value.shape?.kind !== 'circular') add(`${path}.shape.kind`, '当前仅支持圆形截面');
    else if (!Number.isFinite(value.shape.radius) || !(value.shape.radius > tolerance))
      add(`${path}.shape.radius`, '半径必须为有限数字且大于建模公差');
    if (!Number.isFinite(value.rotation)) add(`${path}.rotation`, '旋转角必须为有限数字');
    if (!Number.isFinite(value.spacing) || value.spacing < 0) add(`${path}.spacing`, '间距必须为有限非负数');
    if (typeof value.autoResize !== 'boolean') add(`${path}.autoResize`, '必须为布尔值');
  };
  for (const side of sides) validateOptions(draft.roots?.[side], `roots.${side}`);
  if (!Array.isArray(draft.sections)) add('sections', '必须为数组');
  if (!Array.isArray(draft.relations)) add('relations', '必须为数组');
  const sections = Array.isArray(draft.sections) ? draft.sections : [];
  const relations = Array.isArray(draft.relations) ? draft.relations : [];
  const byId = new Map();
  sections.forEach((section, index) => {
    const path = `sections[${index}]`;
    validateOptions(section, path);
    if (!record(section)) return;
    if (!nonempty(section.id) || byId.has(section.id)) add(`${path}.id`, 'ID 必须非空且唯一');
    else byId.set(section.id, index);
    if (typeof section.label !== 'string') add(`${path}.label`, '必须为字符串');
    if (!sides.includes(section.side)) add(`${path}.side`, '请选择正侧或负侧');
  });
  const parents = sections.map((section, index) => {
    if (!record(section) || section.parentId === null) return undefined;
    const parent = byId.get(section.parentId);
    if (parent !== undefined && parent !== index && sections[parent].side === section.side) return parent;
    add(`sections[${index}].parentId`, '父级必须存在、同侧且不是自身');
    return undefined;
  });
  const visited = new Set();
  for (let index = 0; index < parents.length; index += 1) {
    const chain = new Set();
    let current = index;
    while (current !== undefined && !visited.has(current) && !chain.has(current)) {
      chain.add(current);
      current = parents[current];
    }
    if (chain.has(current)) add(`sections[${current}].parentId`, '嵌套关系不能成环');
    for (const member of chain) visited.add(member);
  }
  const pairs = new Set();
  relations.forEach((relation, index) => {
    const path = `relations[${index}]`;
    if (!record(relation)) { add(path, '必须为对象'); return; }
    for (const side of sides) {
      const key = `${side}Id`;
      if (sections[byId.get(relation[key])]?.side !== side) add(`${path}.${key}`, `端点必须是${sideNames[side]}截面`);
    }
    if (typeof relation.samePosition !== 'boolean') add(`${path}.samePosition`, '必须为布尔值');
    const pair = JSON.stringify([relation.positiveId, relation.negativeId]);
    if (pairs.has(pair)) add(path, '映射端点对不可重复');
    pairs.add(pair);
  });
  return errors;
}

export function parseCaseJson(text) {
  let draft;
  try { draft = JSON.parse(text); }
  catch { return {errors: [fieldError('$', '无法解析 JSON')]}; }
  const errors = validateCaseDraft(draft);
  return errors.length ? {errors} : {draft, errors};
}

export function exportCaseJson(draft) {
  const errors = validateCaseDraft(draft);
  if (errors.length) throw new Error(`${errors[0].path}：${errors[0].message}`);
  return `${JSON.stringify(draft, null, 2)}\n`;
}

const incident = (id, relation) => relation.positiveId === id || relation.negativeId === id;
const caption = (section) => section.label && section.label !== section.id
  ? `${section.label} (${section.id})` : section.id;

const angleDelta = (current, previous) => ((current % 360) - (previous % 360)) % 360;
const sameLayoutOptions = (left, right) => left.shape.kind === right.shape.kind &&
  left.shape.radius === right.shape.radius && left.spacing === right.spacing && left.autoResize === right.autoResize;

function sameLayoutInputs(current, previous) {
  return previous !== null && current.modelingTolerance === previous.modelingTolerance &&
    current.positionTransform.every((value, index) => value === previous.positionTransform[index]) &&
    sides.every((side) => sameLayoutOptions(current.roots[side], previous.roots[side])) &&
    current.sections.length === previous.sections.length &&
    current.sections.every((member, index) => {
      const prior = previous.sections[index];
      return member.id === prior.id && member.side === prior.side && member.parentId === prior.parentId &&
        sameLayoutOptions(member, prior);
    }) &&
    current.relations.length === previous.relations.length &&
    current.relations.every((relation, index) => {
      const prior = previous.relations[index];
      return relation.positiveId === prior.positiveId && relation.negativeId === prior.negativeId &&
        relation.samePosition === prior.samePosition;
    });
}

export function editCaseDraft(draft, action) {
  const next = structuredClone(draft);
  const section = next.sections.find((entry) => entry.id === action.id);
  const reject = (message) => ({draft, error: fieldError('编辑', message)});
  const {type, field, value, side, index} = action;
  if (type === 'root-field') {
    if (field === 'radius') next.roots[side].shape.radius = value;
    else next.roots[side][field] = value;
  }
  if (type === 'section-field' && section) {
    if (field === 'id') {
      if (value === section.id) return {draft};
      if (!nonempty(value) || next.sections.some((entry) => entry.id === value))
        return reject('ID 不能为空或与现有截面重复');
      for (const member of next.sections) if (member.parentId === section.id) member.parentId = value;
      for (const relation of next.relations) {
        if (relation.positiveId === section.id) relation.positiveId = value;
        if (relation.negativeId === section.id) relation.negativeId = value;
      }
    }
    if (field === 'side' && value !== section.side && (section.parentId !== null ||
        next.sections.some((entry) => entry.parentId === section.id) ||
        next.relations.some((relation) => incident(section.id, relation)))) {
      return reject('截面仍有父级、子级或映射引用，请先解除引用再切换侧别');
    }
    if (field === 'radius') section.shape.radius = value;
    else section[field] = value;
  }
  if (type === 'add-section') {
    if (action.parentId != null && !next.sections.some((entry) => entry.id === action.parentId && entry.side === side))
      return reject('子截面必须与父截面同侧');
    const prefix = side === 'positive' ? 'p' : 'n';
    let ordinal = 1;
    const ids = new Set(next.sections.map((entry) => entry.id));
    while (ids.has(`${prefix}-${ordinal}`)) ordinal += 1;
    const id = `${prefix}-${ordinal}`;
    const minimum = Math.max(1, next.modelingTolerance * 2);
    const radius = Number.isFinite(minimum) ? minimum : Number.MAX_VALUE;
    next.sections.push({id, label: id, side, parentId: action.parentId ?? null, ...sectionOptions(radius)});
    return {draft: next, addedId: id};
  }
  if (type === 'delete-section' && section) {
    next.sections = next.sections.filter((entry) => entry.id !== section.id);
    let childCount = 0;
    for (const child of next.sections) {
      if (child.parentId === section.id) { child.parentId = null; childCount += 1; }
    }
    const oldCount = next.relations.length;
    next.relations = next.relations.filter((relation) => !incident(section.id, relation));
    return {draft: next, warning: `已删除 ${section.id}；${childCount} 个直接子截面转为顶层，清理 ${oldCount - next.relations.length} 条映射。`};
  }
  if (type === 'add-relation') {
    const used = new Set(next.relations.map((entry) => JSON.stringify([entry.positiveId, entry.negativeId])));
    for (const positive of next.sections.filter((entry) => entry.side === 'positive')) {
      for (const negative of next.sections.filter((entry) => entry.side === 'negative')) {
        if (used.has(JSON.stringify([positive.id, negative.id]))) continue;
        next.relations.push({positiveId: positive.id, negativeId: negative.id, samePosition: true});
        return {draft: next};
      }
    }
    return reject('没有可新增的端点对；请添加截面或先删除一条现有映射');
  }
  if (type === 'relation-field') next.relations[index][field] = value;
  if (type === 'delete-relation') next.relations.splice(index, 1);
  return {draft: next};
}

function element(tag, text) {
  const node = document.createElement(tag);
  if (text !== undefined) node.textContent = text;
  return node;
}

function inputControl(type, value, label) {
  const input = element('input');
  input.type = type;
  if (type === 'checkbox') input.checked = value;
  else input.value = value ?? '';
  if (type === 'number') input.step = 'any';
  input.setAttribute('aria-label', label);
  return input;
}

function selectControl(value, options, label) {
  const select = element('select');
  select.setAttribute('aria-label', label);
  for (const [id, text, sectionOption] of options) {
    const option = element('option', text);
    option.value = id;
    if (sectionOption) option.dataset.sectionOption = id;
    select.append(option);
  }
  select.value = value ?? '';
  return select;
}

function button(text, activate) {
  const node = element('button', text);
  node.type = 'button';
  node.addEventListener('click', activate);
  return node;
}

const numberValue = (input) => Number.isFinite(input.valueAsNumber) ? input.valueAsNumber : null;
const statusLabels = {
  pending: '等待重新计算', manual: '待手动计算', calculating: '计算中',
  solved: '排布成功', unsolved: '求解失败', invalid: '输入有误',
};

function svgElement(tag, attrs) {
  const node = document.createElementNS('http://www.w3.org/2000/svg', tag);
  for (const [key, value] of Object.entries(attrs)) node.setAttribute(key, String(value));
  return node;
}

// Shape drawing stays separate from IDs, hierarchy, placement and selection.
function drawShape(shape, center, rotation) {
  if (shape.kind !== 'circular') return null;
  const x = center.x, y = -center.y, radius = shape.radius;
  const extent = Math.max(Math.abs(x), Math.abs(y)) + radius;
  if (![x, y, radius, extent].every(Number.isFinite)) return null;
  return {element: svgElement('circle', {cx: x, cy: y, r: radius, transform: `rotate(${-rotation} ${x} ${y})`}), extent};
}

const plots = new WeakMap();
function applyZoom(svg, plot) {
  const extent = plot.extent / plot.zoom;
  svg.setAttribute('viewBox', `${-extent} ${-extent} ${2 * extent} ${2 * extent}`);
}

function renderPlot(side, snapshot, stale, draftIds, selectedId) {
  const panel = lookup(`layout-${side}`);
  const svg = panel.querySelector('svg');
  panel.dataset.stale = String(stale);
  let plot = plots.get(svg);
  if (!plot || plot.snapshot !== snapshot) {
    plot = {snapshot, extent: 0, zoom: plot?.zoom ?? 1};
    plots.set(svg, plot);
    const nodes = snapshot?.nodes.filter((node) => node.side === side) ?? [];
    const rootShape = snapshot?.roots[side].solvedShape;
    const root = rootShape && drawShape(rootShape, {x: 0, y: 0}, 0);
    const members = nodes.map((node) => ({node, drawn: drawShape(node.solvedShape, node.center, node.solvedRotation)}))
      .filter(({drawn}) => drawn !== null).sort((a, b) => b.node.solvedShape.radius - a.node.solvedShape.radius);
    for (const {drawn} of members) plot.extent = Math.max(plot.extent, drawn.extent);
    if (root) plot.extent = Math.max(plot.extent, root.extent);
    plot.extent = Math.min(Number.MAX_VALUE / 32, (plot.extent || 1) * 1.12);
    const e = plot.extent;
    svg.replaceChildren();
    if (snapshot) {
      svg.append(svgElement('line', {class: 'svg-axis', x1: -e, y1: 0, x2: e, y2: 0}),
        svgElement('line', {class: 'svg-axis', x1: 0, y1: -e, x2: 0, y2: e}));
      if (root) {
        const active = nodes.filter((node) => node.parentId === null).length > 1;
        root.element.setAttribute('class', active ? 'side-root' : 'side-root is-reference');
        svg.append(root.element);
      }
      for (const {node, drawn} of members) {
        const circle = drawn.element;
        circle.setAttribute('class', 'section-circle');
        circle.setAttribute('role', 'button');
        circle.setAttribute('aria-label', caption(node));
        circle.dataset.sectionId = node.id;
        circle.dataset.inspectId = node.id;
        svg.append(circle);
      }
    }
    applyZoom(svg, plot);
  }
  for (const control of panel.querySelectorAll('[data-zoom]')) control.disabled = !snapshot;
  for (const circle of svg.querySelectorAll('[data-inspect-id]')) {
    const id = circle.dataset.inspectId;
    const inspectable = draftIds.has(id);
    circle.setAttribute('tabindex', inspectable ? '0' : '-1');
    circle.setAttribute('aria-disabled', String(!inspectable));
    circle.classList.toggle('is-selected', inspectable && id === selectedId);
    circle.setAttribute('aria-pressed', String(inspectable && id === selectedId));
  }
}

const treeDrafts = new WeakMap();
function renderTree(side, draft, selectedId) {
  const tree = lookup(`tree-${side}`);
  if (treeDrafts.get(tree) !== draft) {
    treeDrafts.set(tree, draft);
    const members = draft.sections.filter((entry) => entry.side === side);
    const visited = new Set(), byParent = new Map();
    for (const member of members) {
      if (!byParent.has(member.parentId)) byParent.set(member.parentId, []);
      byParent.get(member.parentId).push(member);
    }
    tree.replaceChildren(element('h3', `${sideNames[side]} · ${byParent.get(null)?.length ?? 0} 个顶层`));
    if (!members.length) tree.append(element('p', '无成员截面'));
    const append = (root) => {
      const todo = [[root, 0]];
      while (todo.length) {
        const [member, depth] = todo.pop();
        if (visited.has(member.id)) continue;
        visited.add(member.id);
        const node = element('button', caption(member));
        node.type = 'button';
        node.className = 'tree-node';
        node.dataset.depth = String(depth);
        node.dataset.sectionId = member.id;
        node.dataset.inspectId = member.id;
        node.style.marginInlineStart = `${Math.min(depth, 8) * .75}em`;
        tree.append(node);
        const children = byParent.get(member.id) ?? [];
        for (let i = children.length - 1; i >= 0; i -= 1) todo.push([children[i], depth + 1]);
      }
    };
    for (const top of byParent.get(null) ?? []) append(top);
    // A temporarily invalid cycle still needs to be inspectable as a draft.
    for (const member of members) append(member);
  }
  for (const button of tree.querySelectorAll('.tree-node'))
    button.setAttribute('aria-pressed', String(button.dataset.inspectId === selectedId));
}

const numberText = (value) => value === null ? '未填写' : String(Number(value.toPrecision(10)));

export function renderDebuggerResult(draft, currentResult, lastSuccess, selectedId) {
  const snapshot = currentResult?.solved ? currentResult : lastSuccess;
  const stale = Boolean(snapshot && !currentResult?.solved);
  const members = new Map(draft.sections.map((member) => [member.id, member]));
  const snapshotNote = document.querySelector('#snapshot-note');
  snapshotNote.textContent = !snapshot ? '尚未得到成功布局' : stale ? '仅显示上次成功布局' : '当前布局';
  for (const side of sides) {
    renderPlot(side, snapshot, stale, members, selectedId);
    renderTree(side, draft, selectedId);
  }
  const mappings = lookup('draft-mappings');
  mappings.replaceChildren(...(draft.relations.length ? draft.relations.map((relation) =>
    element('li', `${relation.positiveId} ↔ ${relation.negativeId} · ${relation.samePosition ? '同位置' : '普通对应'}`))
    : [element('li', '无映射')]));

  const detail = lookup('selection-details').querySelector('div');
  const selected = members.get(selectedId);
  if (!selected) { detail.replaceChildren(element('p', '未选择截面')); return; }
  const input = element('p', `输入 · 半径 ${numberText(selected.shape.radius)} · 旋转 ${numberText(selected.rotation)}° · 间距 ${numberText(selected.spacing)} · 自动调径 ${selected.autoResize ? '开' : '关'}`);
  detail.replaceChildren(element('p', `${sideNames[selected.side]}：${caption(selected)} · 父级 ${selected.parentId ?? '（顶层）'}`), input);
  const solved = snapshot?.nodes.find((node) => node.id === selected.id);
  if (solved) {
    const summary = element('p', `${stale ? '上次成功' : '求解结果'} · 半径 ${numberText(solved.solvedShape.radius)} · 圆心 (${numberText(solved.center.x)}, ${numberText(solved.center.y)}) · 绝对旋转 ${numberText(solved.solvedRotation)}°`);
    summary.dataset.solvedDetails = '';
    detail.append(summary);
  }
}

export async function startDebugger({solveCase, solveRotation}) {
  let draft = initialCase();
  let state = 'pending', currentResult = null, lastSuccess = null;
  let placementDraft = null;
  const rotationEdits = new Map();
  let generation = 0, timer;
  let selectedId = null, importSequence = 0;
  let editMessages = [];
  const auto = lookup('auto-calculate');
  const diagnostics = lookup('diagnostics').querySelector('ul');
  const properties = lookup('section-properties');
  const relationsBody = lookup('relation-editor').tBodies[0];
  const rootControls = sides.flatMap((side) => [
    ['radius', 'shape.radius', 'radius'], ['rotation', 'rotation', 'rotation'],
    ['spacing', 'spacing', 'spacing'], ['autoResize', 'autoResize', 'auto-resize'],
  ].map(([field, path, name]) => {
    const input = lookup(`root-${side}-${name}`);
    input.dataset.path = `roots.${side}.${path}`;
    return {side, field, input};
  }));

  function renderDiagnostics() {
    const errors = currentResult?.errors ?? [];
    const invalidPaths = new Set(errors.map((error) => error.path));
    const messages = [...editMessages, ...errors.map(({path, message}) => `${path}：${message}`),
      ...(currentResult?.diagnostics ?? [])];
    if (currentResult?.counters) {
      for (const side of sides) {
        const {candidateCount, iterationsUsed} = currentResult.counters[side];
        messages.push(`${sideNames[side]}：候选 ${candidateCount}，迭代 ${iterationsUsed}`);
      }
    }
    diagnostics.replaceChildren(...(messages.length ? messages : ['暂无诊断']).map((text) => element('li', text)));
    for (const input of document.querySelectorAll('[data-path]'))
      input.setAttribute('aria-invalid', String(invalidPaths.has(input.dataset.path)));
  }

  function setState(nextState, result = null) {
    state = nextState;
    currentResult = result;
    const output = lookup('status');
    output.dataset.state = state;
    output.textContent = statusLabels[state];
    renderDiagnostics();
    renderDebuggerResult(draft, currentResult, lastSuccess, selectedId);
  }

  function rotationBaseline() {
    let fallbackDelta = 0, preferNegative = false;
    for (const {id, side} of rotationEdits.values()) {
      if (id === undefined &&
          draft.sections.filter((member) => member.side === side && member.parentId === null).length < 2) continue;
      const current = id === undefined ? draft.roots[side] : draft.sections.find((entry) => entry.id === id);
      const previous = id === undefined ? placementDraft.roots[side] :
        placementDraft.sections.find((entry) => entry.id === id);
      if (!current || !previous) continue;
      const delta = angleDelta(current.rotation, previous.rotation);
      if (delta === 0) continue;
      fallbackDelta = delta;
      preferNegative = side === 'negative';
    }
    return {
      placement: lastSuccess.placement,
      preferNegative,
      fallbackDelta,
    };
  }

  function requestSolve(delay, manual = false) {
    clearTimeout(timer);
    const token = ++generation;
    const errors = validateCaseDraft(draft);
    if (errors.length) { setState('invalid', {errors}); return; }
    if (!manual && !auto.checked) { setState('manual'); return; }
    setState('pending');
    timer = setTimeout(() => {
      if (token !== generation) return;
      setState('calculating');
      requestAnimationFrame(() => setTimeout(() => {
        if (token !== generation) return;
        let result;
        try {
          result = solveRotation && lastSuccess?.placement && sameLayoutInputs(draft, placementDraft) ?
            solveRotation(draft, rotationBaseline()) : solveCase(draft);
        }
        catch (error) { result = {status: 'unsolved', diagnostics: [String(error)]}; }
        if (token !== generation) return;
        if (result.solved) {
          lastSuccess?.placement?.delete();
          lastSuccess = result;
          placementDraft = draft;
          rotationEdits.clear();
        }
        setState(result.status, result);
      }, 0));
    }, delay);
  }

  function apply(action, structural = false) {
    const change = editCaseDraft(draft, action);
    const message = change.error?.message ?? change.warning;
    editMessages = message ? [message] : [];
    if (change.draft === draft) { renderDiagnostics(); return false; }
    if (action.field === 'rotation') {
      const member = action.id === undefined ? null : draft.sections.find((entry) => entry.id === action.id);
      const side = action.side ?? member?.side;
      const key = action.id === undefined ? `root:${side}` : `section:${action.id}`;
      const previous = member ? member.rotation : draft.roots[side]?.rotation;
      const lastAngle = Number.isFinite(previous) ? previous : rotationEdits.get(key)?.angle;
      if (Number.isFinite(action.value) &&
          (!Number.isFinite(lastAngle) || angleDelta(action.value, lastAngle) !== 0)) {
        rotationEdits.delete(key);
        rotationEdits.set(key, {id: action.id, side, angle: action.value});
      }
    }
    draft = change.draft;
    if (change.addedId) selectedId = change.addedId;
    if (action.id === selectedId) {
      if (action.type === 'delete-section') selectedId = null;
      if (action.field === 'id') selectedId = action.value;
    }
    if (action.type === 'section-field' && ['id', 'label'].includes(action.field)) {
      const member = draft.sections.find((entry) => entry.id === (action.field === 'id' ? action.value : action.id));
      // ID changes commit on blur. Keep the existing controls so the pending click or Tab can finish.
      const tree = lookup(`section-tree-${member.side}`);
      const row = Array.from(tree.querySelectorAll('li[data-section-id]')).find((entry) => entry.dataset.sectionId === action.id);
      row.dataset.sectionId = member.id;
      row.querySelector('.tree-node').textContent = caption(member);
      properties.dataset.sectionId = member.id;
      properties.querySelector('h3').textContent = `编辑截面 · ${caption(member)}`;
      // The hierarchy is unchanged in both result trees; only the edited node needs a patch.
      for (const side of sides) {
        const resultTree = lookup(`tree-${side}`);
        const node = Array.from(resultTree.querySelectorAll('[data-inspect-id]'))
          .find((entry) => entry.dataset.inspectId === action.id);
        if (node) {
          node.dataset.sectionId = member.id;
          node.dataset.inspectId = member.id;
          node.textContent = caption(member);
        }
        treeDrafts.set(resultTree, draft);
      }
      for (const option of document.querySelectorAll('[data-section-option]')) {
        if (option.dataset.sectionOption !== action.id) continue;
        option.dataset.sectionOption = member.id;
        option.value = member.id;
        option.textContent = caption(member);
      }
    } else if (action.type === 'add-section' || action.type === 'delete-section' || action.field === 'side') {
      renderEditors();
      if (change.addedId)
        lookup(`section-tree-${action.side}`).querySelector('.tree-node[aria-pressed="true"]').focus({preventScroll: true});
    } else if (action.field === 'parentId') {
      renderSectionTrees();
    } else if (action.type === 'add-relation' || action.type === 'delete-relation') {
      renderRelations();
    }
    requestSolve(structural ? 0 : 300);
    return true;
  }

  const options = (side, exclude) => draft.sections.filter((member) => member.side === side && member.id !== exclude)
    .map((member) => [member.id, caption(member), true]);

  function renderSectionTrees() {
    for (const side of sides) {
      const tree = lookup(`section-tree-${side}`);
      const members = draft.sections.filter((member) => member.side === side);
      const roots = element('ul');
      roots.className = 'member-branches';
      if (!members.length) {
        const empty = element('p', '无成员截面');
        empty.className = 'hint';
        tree.replaceChildren(empty);
        continue;
      }
      const children = new Map();
      for (const member of members) {
        if (!children.has(member.parentId)) children.set(member.parentId, []);
        children.get(member.parentId).push(member);
      }
      const visited = new Set();
      const append = (start) => {
        const pending = [[start, roots, 0]];
        while (pending.length) {
          const [member, list, depth] = pending.pop();
          if (visited.has(member.id)) continue;
          visited.add(member.id);
          const item = element('li');
          item.dataset.sectionId = member.id;
          const row = element('div');
          row.className = 'member-row';
          row.style.marginInlineStart = `${Math.min(depth, 8) * .65}rem`;
          const name = button(caption(member), () => selectSection(item.dataset.sectionId));
          name.className = 'tree-node';
          const add = button('+ 子截面', () => apply({type: 'add-section', side, parentId: item.dataset.sectionId}, true));
          add.className = 'add-child';
          row.append(name, add);
          item.append(row);
          list.append(item);
          const descendants = children.get(member.id)?.filter((child) => !visited.has(child.id));
          if (descendants?.length) {
            const branch = element('ul');
            branch.className = 'member-branches';
            item.append(branch);
            for (let i = descendants.length - 1; i >= 0; i -= 1) pending.push([descendants[i], branch, depth + 1]);
          }
        }
      };
      for (const member of children.get(null) ?? []) append(member);
      // Keep cyclic drafts visible and editable while the validator reports the error.
      for (const member of members) append(member);
      tree.replaceChildren(roots);
    }
    updateEditorSelection();
  }

  function updateEditorSelection() {
    for (const node of document.querySelectorAll('.member-row .tree-node'))
      node.setAttribute('aria-pressed', String(node.closest('li').dataset.sectionId === selectedId));
  }

  function renderProperties() {
    const index = draft.sections.findIndex((entry) => entry.id === selectedId);
    if (index < 0) {
      delete properties.dataset.sectionId;
      properties.replaceChildren(element('p', '选择一个截面编辑参数。'));
      return;
    }
    const member = draft.sections[index];
    properties.dataset.sectionId = member.id;
    const title = element('h3', `编辑截面 · ${caption(member)}`);
    const fields = element('div');
    fields.className = 'property-fields';
    const field = (name, text, control, path = name) => {
      control.dataset.field = name;
      control.dataset.path = `sections[${index}].${path}`;
      const label = element('label', text);
      if (control.type === 'checkbox') label.className = 'check';
      label.append(control);
      fields.append(label);
      return control;
    };
    const edit = (name, value, structural) => apply({type: 'section-field', id: properties.dataset.sectionId,
      field: name, value}, structural);
    const id = field('id', '截面 ID', inputControl('text', member.id, '截面 ID'));
    id.addEventListener('keydown', (event) => { if (event.key === 'Enter') { event.preventDefault(); id.blur(); } });
    id.addEventListener('change', () => { if (!edit('id', id.value, true)) id.value = properties.dataset.sectionId; });
    const label = field('label', '名称', inputControl('text', member.label, '名称'));
    label.addEventListener('input', () => edit('label', label.value));
    const side = field('side', '侧别', selectControl(member.side, sides.map((key) => [key, sideNames[key]]), '侧别'));
    side.addEventListener('change', () => {
      if (!edit('side', side.value, true)) side.value = member.side;
      else properties.querySelector('[data-field="side"]').focus();
    });
    const parent = field('parentId', '父级', selectControl(member.parentId,
      [['', '顶层'], ...options(member.side, member.id)], '父级'));
    parent.addEventListener('change', () => edit('parentId', parent.value || null, true));
    for (const [name, value, text, path] of [['radius', member.shape.radius, '半径', 'shape.radius'],
      ['rotation', member.rotation, '旋转 (°)'], ['spacing', member.spacing, '间距']]) {
      const input = field(name, text, inputControl('number', value, text), path);
      input.addEventListener('input', () => edit(name, numberValue(input)));
    }
    const autoResize = field('autoResize', '自动调径', inputControl('checkbox', member.autoResize, '自动调径'));
    autoResize.addEventListener('change', () => edit('autoResize', autoResize.checked, true));
    const hint = element('p', '编辑 ID 后按 Enter 或离开输入框，已有引用会同步更新。');
    hint.className = 'hint';
    const remove = button('删除截面', () => apply({type: 'delete-section', id: properties.dataset.sectionId}, true));
    properties.replaceChildren(title, hint, fields, remove);
  }

  function renderRelations() {
    relationsBody.replaceChildren(...draft.relations.map((relation, index) => {
      const row = element('tr');
      for (const [field, choices, text] of [['positiveId', options('positive'), '正侧端点'],
        ['negativeId', options('negative'), '负侧端点'], ['samePosition', [['true', '同位置'], ['false', '普通对应']], '关系']]) {
        const control = selectControl(relation[field], choices, text);
        control.dataset.field = field;
        control.dataset.path = `relations[${index}].${field}`;
        control.addEventListener('change', () => apply({type: 'relation-field', index, field,
          value: field === 'samePosition' ? control.value === 'true' : control.value}, true));
        row.insertCell().append(control);
      }
      row.insertCell().append(button('删除', () => apply({type: 'delete-relation', index}, true)));
      return row;
    }));
  }

  function renderEditors() {
    for (const {side, field, input} of rootControls) {
      const options = draft.roots[side];
      if (field === 'autoResize') input.checked = options.autoResize;
      else input.value = (field === 'radius' ? options.shape.radius : options[field]) ?? '';
    }
    renderSectionTrees();
    renderProperties();
    renderRelations();
  }

  for (const side of sides) {
    lookup(`add-section-${side}`).addEventListener('click', () => apply({type: 'add-section', side}, true));
  }
  for (const {side, field, input} of rootControls) {
    const toggle = field === 'autoResize';
    input.addEventListener(toggle ? 'change' : 'input', () => {
      apply({type: 'root-field', side, field, value: toggle ? input.checked : numberValue(input)}, toggle);
    });
  }
  lookup('add-relation').addEventListener('click', () => apply({type: 'add-relation'}, true));
  const calculate = lookup('calculate');
  calculate.disabled = false;
  calculate.addEventListener('click', () => requestSolve(0, true));
  auto.addEventListener('change', () => {
    if (auto.checked || state === 'pending' || state === 'calculating') requestSolve(0);
  });

  const importFile = lookup('case-import');
  importFile.addEventListener('change', async () => {
    const file = importFile.files[0];
    importFile.value = '';
    if (!file) return;
    const token = ++importSequence;
    let parsed;
    try { parsed = parseCaseJson(await file.text()); }
    catch { parsed = {errors: [fieldError('$', '无法读取案例文件')]}; }
    if (token !== importSequence) return;
    if (!parsed.draft) {
      editMessages = parsed.errors.map(({path, message}) => `导入未应用 · ${path}：${message}`);
      renderDiagnostics();
      return;
    }
    draft = parsed.draft;
    placementDraft = null;
    rotationEdits.clear();
    if (!draft.sections.some((entry) => entry.id === selectedId)) selectedId = null;
    editMessages = [];
    renderEditors();
    requestSolve(0);
  });

  lookup('case-export').addEventListener('click', () => {
    let json;
    try { json = exportCaseJson(draft); }
    catch (error) { editMessages = [`导出未应用 · ${error.message}`]; renderDiagnostics(); return; }
    const url = URL.createObjectURL(new Blob([json], {type: 'application/json'}));
    const link = element('a');
    link.href = url;
    link.download = 'section-layout-case.json';
    link.click();
    setTimeout(() => URL.revokeObjectURL(url), 0);
  });

  function selectSection(id) {
    if (selectedId === id || !draft.sections.some((entry) => entry.id === id)) return;
    selectedId = id;
    updateEditorSelection();
    renderProperties();
    renderDiagnostics();
    renderDebuggerResult(draft, currentResult, lastSuccess, selectedId);
  }

  const inspect = (event) => {
    const node = event.target.closest('[data-inspect-id]');
    if (node) selectSection(node.dataset.inspectId);
  };
  for (const side of sides) {
    lookup(`tree-${side}`).addEventListener('click', inspect);
    const panel = lookup(`layout-${side}`);
    const svg = panel.querySelector('svg');
    svg.addEventListener('click', inspect);
    svg.addEventListener('keydown', (event) => {
      if (event.key === 'Enter' || event.key === ' ') { event.preventDefault(); inspect(event); }
    });
    for (const control of panel.querySelectorAll('[data-zoom]')) {
      control.addEventListener('click', () => {
        const plot = plots.get(svg);
        const action = control.dataset.zoom;
        plot.zoom = action === 'fit' ? 1 : Math.min(8, Math.max(0.5, plot.zoom * (action === 'in' ? 1.25 : 0.8)));
        applyZoom(svg, plot);
      });
    }
  }
  document.querySelector('main').inert = false;
  renderEditors();
  requestSolve(0);
}
