module.exports = function(fileInfo, api) {
  const j = api.jscodeshift;
  const root = j(fileInfo.source);

  root.find(j.JSXText)
    .filter(path => path.value.value.trim().length > 0)
    .replaceWith(path => {
      const text = path.value.value.trim();
      return j.jsxExpressionContainer(
        j.callExpression(j.identifier('t'), [j.literal(text)])
      );
    });

  return root.toSource({ quote: 'single' });
};
