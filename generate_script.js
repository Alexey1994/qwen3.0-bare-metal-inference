var size = '12'//'22+(%size1%+511)/512+(%size2%+511)/512+(%size3%+511)/512'
var sizeIndex = 1
var allSources = []
var allTargets = []

function next() {
	console.log()
	size += `+(%size${sizeIndex}%+507)/508`
	++sizeIndex
}

function file(from, to) {
	if(allSources.length>0 && allSources.length % 31 === 0) {
		console.log(`@echo db ${Array(12).fill(1).join(',')} >> bin/fs.asm`)
	}

	allSources.push(from)
	allTargets.push(to)

	var toTarget = Array(12).fill(0)

	to.split('').forEach((character, index) => {
		toTarget[index] = character.charCodeAt()
	})

	console.log(`@for %%I in (${from}) do @set "size${sizeIndex}=%%~zI"`)
	console.log(`@echo db ${toTarget.join(',')} dd ${size} >> bin/fs.asm`)
	console.log(`@load ${from} | index > bin/${to}.index`)
	next()
}


console.log('@echo ; > bin/fs.asm')

file(`bin/system.bin`, `system`)


file(`model/tokenizer.bin`, `tokenizer`)

file(`model/model/embed_tokens/weight.151936x1024.BF16`, `embed`)

for(var i=0; i<28; ++i) {
	file(`model/model/layers/${i}/input_layernorm/weight.1024.BF16`, `L${i}norm_in`)
	file(`model/model/layers/${i}/self_attn/q_proj/weight.2048x1024.BF16`, `L${i}q`)
	file(`model/model/layers/${i}/self_attn/k_proj/weight.1024x1024.BF16`, `L${i}k`)
	file(`model/model/layers/${i}/self_attn/v_proj/weight.1024x1024.BF16`, `L${i}v`)
	file(`model/model/layers/${i}/self_attn/o_proj/weight.1024x2048.BF16`, `L${i}o`)
	file(`model/model/layers/${i}/self_attn/q_norm/weight.128.BF16`, `L${i}q_n`)
	file(`model/model/layers/${i}/self_attn/k_norm/weight.128.BF16`, `L${i}k_n`)
	file(`model/model/layers/${i}/mlp/gate_proj/weight.3072x1024.BF16`, `L${i}gate`)
	file(`model/model/layers/${i}/mlp/up_proj/weight.3072x1024.BF16`, `L${i}up`)
	file(`model/model/layers/${i}/mlp/down_proj/weight.1024x3072.BF16`, `L${i}down`)
	file(`model/model/layers/${i}/post_attention_layernorm/weight.1024.BF16`, `L${i}norm_out`)
}

file(`model/model/norm/weight.1024.BF16`, `norm`)


console.log(`@load bin/fs.asm | a386 | index > bin/fs.bin`)
console.log(`@load ${allTargets.map(to => 'bin/'+to+'.index').join(' ')} > bin/fsdata.bin`)
console.log(`@load bin/bootloader.bin bin/fs.bin bin/fsdata.bin | reindex > bin/storage`)