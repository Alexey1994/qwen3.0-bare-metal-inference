var fs = require('fs')


var tokenizer = JSON.parse(fs.readFileSync('./qwen3-0.6b/tokenizer.json', 'utf-8'))

var vocab = tokenizer.model.vocab
var merges = tokenizer.model.merges

var specialTokens = tokenizer["added_tokens"]



specialTokens.forEach(token => {
	vocab[token.content] = token.id
})


var vocabData = []
Object.keys(vocab).forEach(vocabValue => {
	var token = vocab[vocabValue]
	vocabData[token] = vocabValue
})

var mergesData = []
merges.forEach(merge => {
	var token1 = vocab[merge[0]]
	var token2 = vocab[merge[1]]
	var token3 = vocab[merge[0] + merge[1]]
	mergesData.push([token1, token2, token3])
})


var buffer = []

buffer.push(Buffer.alloc(4))
buffer[0].writeUInt32LE(vocabData.length, 0)

buffer.push(Buffer.alloc(4))
buffer[1].writeUInt32LE(mergesData.length, 0)


function bytes_to_unicode() {
	const bs = []
	const cs = []

	// Printable ASCII
	for (let b = 33; b <= 126; b++) bs.push(b)
	// Latin-1 supplement
	for (let b = 161; b <= 172; b++) bs.push(b)
	for (let b = 174; b <= 255; b++) bs.push(b)

	const initial_bs = [...bs]
	cs.push(...initial_bs)

	let n = 0;
	for (let b = 0; b < 256; b++) {
		if (!initial_bs.includes(b)) {
			bs.push(b)
			cs.push(256 + n)
			n++
		}
	}

	const mapping = {};
	for (let i = 0; i < bs.length; i++) {
		mapping[bs[i]] = String.fromCodePoint(cs[i])
	}
	return mapping
}

const byteToUnicode = bytes_to_unicode()
const unicodeToByte = {}
for (const [byte, char] of Object.entries(byteToUnicode)) {
    unicodeToByte[char] = parseInt(byte)
}


vocabData.forEach((word, token) => {
	var bytes = Buffer.from(word.split('').map(c => unicodeToByte[c]))

	buffer.push(Buffer.alloc(4))
    buffer[buffer.length-1].writeUInt32LE(bytes.length, 0)

    buffer.push(bytes)
})

mergesData.forEach(merge => {
	buffer.push(Buffer.alloc(12))
	buffer[buffer.length-1].writeUInt32LE(merge[0], 0)
	buffer[buffer.length-1].writeUInt32LE(merge[1], 4)
	buffer[buffer.length-1].writeUInt32LE(merge[2], 8)
})

fs.writeFileSync('tokenizer.bin', Buffer.concat(buffer))