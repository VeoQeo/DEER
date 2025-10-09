	.set		MAGIC, 0xE85250D6	
	.set		FLAGS, 0		
	.set		CHECKSUM, -(MAGIC + FLAGS + (mbh_end - mbh_start))

	.section	.multiboot
	.align		4
mbh_start:
	.long		MAGIC
	.long		FLAGS
	.long		mbh_end - mbh_start
	.long		CHECKSUM

	.word		0					
	.word		0					
	.long		8					
mbh_end:

