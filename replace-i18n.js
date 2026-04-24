module.exports = function(fileInfo, api) {
  const j = api.jscodeshift;
  const map = { 'Floors': 'floor_count', 'Buildings': 'building_count' };
  return j(fileInfo.source)
    .find(j.Literal)
    .replaceWith(p => map[p.node.value]
      ? j.callExpression(j.identifier('t'), [j.literal(map[p.node.value])])
      : p.node
    ).toSource();
};
