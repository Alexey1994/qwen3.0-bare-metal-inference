var fs = require('fs')
var path = require('path')


var data = fs.readFileSync('./qwen3-0.6b/model.safetensors')
var headerSize = Number(data.readBigUInt64LE(0))
var header = JSON.parse(data.slice(8, 8 + headerSize).toString('utf-8'))


Object.keys(header).forEach(key => {
	console.log(key, header[key])

	if(key === '__metadata__') {
		return
	}

	var fileName = 'model/'
		+
		key.replaceAll('.', '/')
		+
		'.'
		+
		header[key].shape.join('x')
		+
		'.'
		+
		header[key].dtype

	var dir = path.dirname(fileName)
	
	if(!fs.existsSync(dir)) {
		fs.mkdirSync(dir, { recursive: true })
	}

	fs.writeFileSync(
		fileName,

		data.slice(
			8 + headerSize + header[key].data_offsets[0],
			8 + headerSize + header[key].data_offsets[1]
		)
	)
})