const path = require('path');
const webpack = require('webpack');

module.exports = {
    entry : ['babel-polyfill', './src/demo.js'],
    output: {
        filename: 'bundle.js',
        library: 'thymio'
    },
    devtool: 'sourcemap',
    mode   : 'production',
    module: {
        rules: [
            {
                test: /\.js$/,
                loader: 'babel-loader',
                options: {
                    presets: ['env']
                },
                exclude: /node_modules/
            },
            {
                //test: require.resolve('flatbuffers'),
                //use: 'imports-loader?this=>window!./flatbuffers'
            }
        ]
    }
}
