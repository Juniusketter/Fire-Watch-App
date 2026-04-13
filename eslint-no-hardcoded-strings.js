module.exports = {
  plugins: ["react"],
  rules: {
    "react/jsx-no-literals": ["error", { "noStrings": true, "ignoreProps": true }]
  }
};
