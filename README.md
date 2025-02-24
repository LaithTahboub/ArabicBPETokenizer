This is part of a research project supervised by Dr. Nayeem to train an LLM on Islamic knowledge. This LLM will be trained primarily or solely on authentic Arabic texts. Before we can perform this training, we need a strong Arabic tokenizer that effectively produces a vocabulary that can be used later during training and inference. 

To use the tokenizer, simple download this repo locally and run the command ```./BPE```. This will open the tokenizer CLI. Here are the commands you can use:
- ```train <text_file> <vocab_size> <save_dir>```: Trains the model given some input text file, a vocab size, and a directory that will cache the results. This will also allow you to encode and decode using the tokenizer you just trained without loading it again.
- ```load <directory>```: Reads from a a directory that should contain ```vocab.txt``` and ```merges.txt``` and loads the tokenizer.
- ```encode "<text>"```: Encodes a string into a list of token ids.
- ```decode <comma_separated_ids>```: Decodes a list of token ids into a string.