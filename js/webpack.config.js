const path = require('path');
const webpack = require('webpack');
const nodeConfig = {
    target: 'node',
    entry : ['babel-polyfill', './src/demo.js'],
    output: {
        filename: 'thymio-api-node.js',
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
            }
        ]
    }
};

const browserConfig = {
    target: 'web', // <=== can be omitted as default is 'web'
    entry : ['babel-polyfill', './src/demo.js'],
    output: {
        filename: 'thymio-api.js',
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
            }
        ]
    }
};

module.exports = [ nodeConfig, browserConfig ];
