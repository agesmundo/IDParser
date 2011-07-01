import java.io.BufferedReader;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.io.Reader;
import java.io.Writer;
import java.nio.charset.Charset;
import java.util.ArrayList;
import java.util.List;

public class PrepareOutputC08 {
	static final Charset defaultCharset = Charset.forName("UTF-8");
	static final int senseColumnId = 8;
	static final int argumentsColumnId = senseColumnId+1;

	public static void main(String args[]) {
		String use = "java PrepareOutputC08 <input_file_name> <output_file_name>";
		if (args.length !=2) {
			System.out.println(use);
			return;
		}

		String inputFileName = args[0];
		String outputFileName = args[1];

		List<List<List<String>>> tokenizedSentences = getTokenizedSentences(getListString(inputFileName));// [sentence][line][token]
		substituteCharCode(tokenizedSentences);
		substituteWave(tokenizedSentences);
		List<List<List<String>>> outputTokenizedSentences = setArgumentsColumn(tokenizedSentences);
		print(outputFileName, getString(outputTokenizedSentences));

	}

	private static String getString(List<List<List<String>>> tokenizedSentences) {
		StringBuffer rtn = new StringBuffer("");
		boolean firstSentence = true;
		for (List<List<String>> tokenizedSentence : tokenizedSentences) {
			if (firstSentence) {
				firstSentence = false;
			} else {
				rtn.append("\n\n");
			}
			boolean firstLine = true;
			for (List<String> lineTokens : tokenizedSentence) {
				if (firstLine) {
					firstLine = false;
				} else {
					rtn.append("\n");
				}
				boolean firstToken = true;
				for (String token : lineTokens) {
					if (firstToken) {
						firstToken = false;
					} else {
						rtn.append("\t");
					}
					rtn.append(token);
				}

			}
		}

		return rtn.toString();
	}

	private static List<List<List<String>>> setArgumentsColumn(
			List<List<List<String>>> tokenizedSentences) {
		List<List<List<String>>> outputTokenizedSentences = new ArrayList<List<List<String>>>();
		List<List<String>> outputTokenizedSentence;
		List<String> outputLineTokens;
		int predicatesNumber;
		for (List<List<String>> tokenizedSentence : tokenizedSentences) {
			predicatesNumber = countPredicaters(tokenizedSentence);
			outputTokenizedSentence = new ArrayList<List<String>>();
			for (List<String> lineTokens : tokenizedSentence) {
				outputLineTokens = new ArrayList<String>();

				// copy first columns
				for (int i = 0; i <= senseColumnId; i++) {
					outputLineTokens.add(lineTokens.get(i));
				}

				// add colums for arguments
				for (int i = 0; i < predicatesNumber; i++) {
					outputLineTokens.add("_");
				}

				outputTokenizedSentence.add(outputLineTokens);
			}
			int columnId = argumentsColumnId;

			for (List<String> lineTokens : tokenizedSentence) {

				if (!lineTokens.get(senseColumnId).equals("_")) {

					for (int i = argumentsColumnId; i < lineTokens.size(); i++) {
						int lineID = Integer.parseInt(lineTokens.get(i++));
						String argLabel = lineTokens.get(i);

						outputTokenizedSentence.get(lineID - 1).set(columnId,
								argLabel);
					}

					columnId++;
				}

			}
			
			outputTokenizedSentences.add(outputTokenizedSentence);
		}

		return outputTokenizedSentences;
	}

	private static int countPredicaters(List<List<String>> tokenizedSentence) {
		int count = 0;
		for (List<String> lineTokens : tokenizedSentence) {
			if (!lineTokens.get(senseColumnId).equals("_")) {
				count++;
			}
		}
		return count;
	}

	private static void substituteWave(
			List<List<List<String>>> tokenizedSentences) {
		for (List<List<String>> tokenizedSentence : tokenizedSentences) {
			int lineNumber=0;
			for (List<String> lineTokens :tokenizedSentence) {

				lineNumber++;

				String senseTokens[] = lineTokens.get(senseColumnId).split("~");
				if (senseTokens.length==1) {
					continue;
				}
				else if (senseTokens.length==2){
					lineTokens.set(senseColumnId, senseTokens[0]);
					boolean inserted =false;
					for (int i = argumentsColumnId ; i<lineTokens.size();i+=2){
						int argLineNumber = Integer.parseInt(lineTokens.get(i));
						if(argLineNumber>lineNumber){
							lineTokens.add(i, senseTokens[1]);
							lineTokens.add(i, ""+lineNumber);
							inserted=true;
							break;
						}
						else if (argLineNumber>lineNumber){
							throw new RuntimeException("Two labels for the same semantic link found: "+lineTokens.get(senseColumnId));
						}
					}
					if(!inserted){
						lineTokens.add(""+lineNumber);
						lineTokens.add(senseTokens[1]);	
						inserted=true;
					}
				}
				else {
					throw new RuntimeException("Sense splitted with ~ has more then 2 tokens: "+lineTokens.get(senseColumnId));
				}


			}
		}
	}

	private static void substituteCharCode(
			List<List<List<String>>> tokenizedSentences) {
		StringBuffer newSense;
		StringBuffer charCode;
		for (List<List<String>> tokenizedSentence : tokenizedSentences) {
			for (List<String> lineTokens : tokenizedSentence) {
				if (lineTokens.get(senseColumnId).equals("_")) {
					continue;
				}
				char[] sense = lineTokens.get(senseColumnId).toCharArray();
				newSense = new StringBuffer("");

				for (int i = 0; i < sense.length; i++) {
					if (sense[i] == '&'
						&& i + 3 < sense.length
						&& sense[i + 1] == '#'
							&& (sense[i + 2] == '0' || sense[i + 2] == '1'
								|| sense[i + 2] == '2'
									|| sense[i + 2] == '3'
										|| sense[i + 2] == '4'
											|| sense[i + 2] == '5'
												|| sense[i + 2] == '6'
													|| sense[i + 2] == '7'
														|| sense[i + 2] == '8' || sense[i + 2] == '9')) {

						charCode = new StringBuffer("");
						i += 2;
						while (sense[i] != ';') {
							charCode.append(sense[i++]);
						}
						int newCharCode = Integer.parseInt(charCode.toString());
						char newChar = (char) newCharCode;
						newSense.append(newChar);
					} else {
						newSense.append(sense[i]);
					}
				}

				lineTokens.set(senseColumnId, newSense.toString());
			}
		}
	}

	private static List<List<List<String>>> getTokenizedSentences(
			List<String> inLines) {
		List<List<List<String>>> tokenizedSentences = new ArrayList<List<List<String>>>();
		List<List<String>> tokenizedSentence = new ArrayList<List<String>>();

		boolean isCurrentLineEmpty = (inLines.get(0).trim().length() == 0);
		boolean isNextLineEmpty;

		for (int i = 0; i < inLines.size(); i++) {
			String currentLine = inLines.get(i).trim();

			if (!((i + 1) < inLines.size())
					|| inLines.get(i + 1).trim().length() == 0) {
				isNextLineEmpty = true;
			} else {
				isNextLineEmpty = false;
			}

			// tokenize line and add it to the current sentence
			String[] tokens = currentLine.split("\t");

			if (tokens.length >= senseColumnId) {
				tokenizedSentence.add(arrayAsList(tokens));
			} else if (tokens.length != 1 || !tokens[0].equals("")) {
				throw new RuntimeException("Attribute missing:\n" + currentLine);
			}

			// check if it is at the end of a sentence
			if (!isCurrentLineEmpty && isNextLineEmpty) {
				// at the end of a sentence
				tokenizedSentences.add(tokenizedSentence);
				tokenizedSentence = new ArrayList<List<String>>();
			} else if (isCurrentLineEmpty && !isNextLineEmpty) {
				if (tokenizedSentence.size() != 0) {
					throw new RuntimeException("tokenizedSentence not Empty");
				}
			}

			isCurrentLineEmpty = isNextLineEmpty;
		}
		return tokenizedSentences;
	}

	private static List<String> arrayAsList(String[] tokens) {
		List <String> rtn = new ArrayList<String> ();
		for (String token : tokens ){
			rtn.add(token);
		}
		return rtn;
	}

	static List<String> getListString(String fileName) {

		Reader reader = null;
		try {
			reader = new BufferedReader(new InputStreamReader(
					new FileInputStream(fileName),
					PrepareOutputC08.defaultCharset));
		} catch (FileNotFoundException e) {
			e.printStackTrace();
		}
		List<String> lines = new ArrayList<String>();
		try {
			BufferedReader in = new BufferedReader(reader);
			String line = in.readLine();
			while (line != null) {
				lines.add(line);
				line = in.readLine();
			}
			in.close();
		} catch (IOException e) {
			System.err.println(e.toString());
		}
		return lines;
	}

	static void print(String fileName, String content) {
		try {
			Writer out = new OutputStreamWriter(new FileOutputStream(fileName),
					defaultCharset);// write out file
			out.write(content);
			out.close();
		} catch (IOException e) {
			System.out.println(e);
		}
	}

}
