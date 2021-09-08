const version = require('./package.json').version
var fs = require('fs');

const dest = process.argv[2]
const exec = require('child_process').exec;


let files = require("glob").glob.sync("mobsya-association-thymio-api-*.tgz");
files.forEach((file) => {
    console.log(`Deleting ${file}`);
    fs.unlinkSync(file)
})

exec('npm pack', function callback(error, stdout, stderr){
    if (error) {
        console.error(`exec error: ${error}`);
        return;
    }
    console.log(`stdout: ${stdout}`);
    console.log(`stderr: ${stderr}`);

    const src = `mobsya-association-thymio-api-${version}.tgz`

    console.log(`${src} => ${dest}`)

    fs.copyFileSync(`${src}`, `${dest}`)
    process.exit(0);
});