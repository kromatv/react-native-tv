module.exports = function babelConfig(api) {
  api.cache(true);
  // This app loads react-native-overrides/ from the repository rather than
  // node_modules, so babel-preset-expo warns about the overrides' deep imports.
  return { presets: [['babel-preset-expo', { disableDeepImportWarnings: true }]] };
};
