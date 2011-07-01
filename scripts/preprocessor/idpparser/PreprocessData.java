package idpparser;
import java.util.*;
import java.io.*;

/**
 * Title:               PreprocessData<br>
 * Description:         <br>
 * Copyright:           Copyright (c) Uni of Geneva<br>
 * Company:             Uni of Geneva<br>
 * @author              Ivan Titov
 * @version             $Id$
 */
public class PreprocessData {
    /** cvs revision id */
    private static final String CVS_ID = "$Id$";
    private static final String UNKNOWN = "#UNKNOWN#";
    private static final String ROOT_DEP = "ROOT";

    private static int BANK_NUM = 2;
    
    public static class PosRolePair {
        public int a;
        public String role;
        public String toString() {
            return a + "\t" + role;
        }
    }

    //------------------------ Subclasses ----------------------------
    public static class Record {
        int idx, bank;
        String word, lemma, pos, cpos, feat, sense;
        List elemFeatures = new LinkedList();
        List args = new LinkedList();

        int head = -1;
        String rel = null;
        
        public static Record parseRecord(String line) {
            StringTokenizer st = new StringTokenizer(line);
            Record rec = new Record();
            rec.idx = Integer.parseInt(st.nextToken());
            rec.word = st.nextToken();
            rec.lemma = st.nextToken();
            rec.cpos = st.nextToken();
            rec.pos = st.nextToken();
            rec.feat = st.nextToken();
            rec.bank = Integer.parseInt(st.nextToken());
            if (st.hasMoreTokens()) {
                rec.head = Integer.parseInt(st.nextToken());
                rec.rel = st.nextToken();
                rec.sense = st.nextToken();
                
                while (st.hasMoreTokens()) {
                    PosRolePair posRole = new PosRolePair();
                    posRole.a = Integer.parseInt(st.nextToken());
                    posRole.role = st.nextToken();
                    rec.args.add(posRole);
                }

            }

            StringTokenizer stPipe = new StringTokenizer(rec.feat, "|");
            while  (stPipe.hasMoreTokens()) {
                rec.elemFeatures.add(stPipe.nextToken());
            }
            
            return rec;
        }
        public String toString() {
            String s =  idx + "\t" + word + "\t" + lemma + "\t" + cpos + "\t" 
                + pos + "\t" + feat + "\t" + bank;
            if (head >= 0) {
                s += "\t" + head + "\t" + rel + "\t" + sense;
                Iterator  posRoles = args.iterator();
                while (posRoles.hasNext()) {
                    s += "\t" + posRoles.next();
                }
            }
            return s;
        }
    }
 
    public static class CutOff {
        //cutoff and unknown cutoff for input features
        int main, unkn;
        public CutOff() {
            this.main = 0;
            this.unkn = 0; 
        }
        public CutOff(int main, int unkn) {
            this.main = main;
            this.unkn = unkn;
        }
        public String toString() {
            return main +"/" + unkn;
        }
    }


    public static class FixedList {
        boolean skip = false;
        String skipName = null;

        public FixedList() {

        }
        
        public FixedList(String name) {
            skip = true;
            skipName = name;
        }
        
        Map map = new TreeMap();
        int cnt = 0;
        
        int size() {
            return map.size();
        }

        void occurred(String name) {
            if (skip && name.equals(skipName)) {
                //do nothing
                return;
            }
            
            if (!map.containsKey(name)) {
                map.put(name, new Integer(cnt++));
            }
        }
        int getId(String name) throws NoSuchFieldException {
            if (skip && name.equals(skipName)) {
                //return negative
                return -1;
            }
            if (!map.containsKey(name)) {
                throw new NoSuchFieldException("Unknown field '" + name + "'");
            }
            return ((Integer) map.get(name)).intValue();
        }

        boolean contains(String name) {
            return (skip && name.equals(skipName)) || map.containsKey(name);
        }
        
        LinkedList getNamesSorted() {
            Iterator it = map.keySet().iterator();
            LinkedList list = new LinkedList();
            SortedMap sm = new TreeMap();
            while (it.hasNext()) {
                Object o =  it.next();
                sm.put(map.get(o), o);
            }
            it = sm.keySet().iterator();
            while (it.hasNext()) {
                list.addLast(sm.get(it.next()));
            }
            return list;            
        }
    }

    static String removeMiddle(String x, int sep) {
        int i1 = x.indexOf(sep);
        int i2 = x.lastIndexOf(sep);
        if (i1 == i2 || i1 < 0 || i2 < 0) {
            return x;
        } else {
            return x.substring(0, i1) + x.substring(i2, x.length());
        }
    }
    
    public  class PosList {
        Map map = new TreeMap();
        int nextId = 0;
        public class Pos implements Comparable {
            String name;
            int id;
            FeatList inFeatList, outFeatList;
            Pos (String name, int id) {
                this.name = name;
                this.id = id;
                inFeatList = new FeatList(featIn, wordIn);
                outFeatList = new FeatList(featOut, wordOut);
            }
            void occurred(Record rec) {
                inFeatList.occurred(rec);
                outFeatList.occurred(rec);
            }
            public boolean equals(Object o) {
                if (! (o instanceof Pos)) {
                    return false;
                }
                Pos oPos = (Pos) o;
                return id == oPos.id;
            }
            public int compareTo(Object o) {
                Pos oPos = (Pos) o;
                if (id < oPos.id) {
                    return -1;
                } else if (id == oPos.id) {
                    return 0;
                } else {
                    return 1;
                }
            }
            public void process() {
                inFeatList.process();
                outFeatList.process();
            }
        } //Pos class
        
        void occurred(Record rec) {
            String key;
            key = removeMiddle(rec.pos, '_');
            if (map.containsKey(key)) {
                Pos pos =  (Pos) map.get(key);
                pos.occurred(rec);
            } else {
                Pos pos = new Pos(key, nextId++);
                pos.occurred(rec);
                map.put(key, pos);
            }
        }
        private Pos getPos(String name) throws NoSuchFieldException {
            String pos = removeMiddle(name, '_');
            if (!map.containsKey(pos)) {
                System.out.println(map.keySet());
                throw new NoSuchFieldException("Unknown POS '" + pos + "'");
            }
            return (Pos) map.get(pos);
        }
        
        int getId(String name) throws NoSuchFieldException {
            return getPos(name).id;
        }


        FeatList getOutFeatList(String name) throws NoSuchFieldException {
            return getPos(name).outFeatList;
        }

        FeatList getInFeatList(String name) throws NoSuchFieldException {
            return getPos(name).inFeatList;
        }

        
        LinkedList getNamesSorted() {
            Iterator it = map.keySet().iterator();
            LinkedList posList = new LinkedList();
            SortedMap sm = new TreeMap();
            while (it.hasNext()) {
                Object pos =  it.next();
                sm.put(map.get(pos), pos);
            }
            it = sm.keySet().iterator();
            while (it.hasNext()) {
                posList.addLast(sm.get(it.next()));
            }
            return posList;            
        }

        int size() {
            return map.size();
        }
        void process() {
            Iterator it = map.keySet().iterator();
            while (it.hasNext()) {
                String name = (String) it.next();
                Pos pos = (Pos) map.get(name);
                pos.process();     
            }
        }
    }

    public class FeatList {
        CutOff featCf, wordCf;
        Map map = new TreeMap();
        int nextId = 1;
        public class Feat implements Comparable {
            String name;
            int id;
            int cnt = 0;
            WordList wordList;
            Feat(String name, int id) {
                this.name = name;
                this.id = id;
                wordList = new WordList(wordCf);
            }
            void occurred(Record rec) {
                cnt++;
                String w = rec.word;
                if (toLowerCase) {
                    w = w.toLowerCase();
                }
                wordList.occurred(w);    
            }
            public boolean equals(Object o) {
                if (! (o instanceof Feat)) {
                    return false;
                }
                Feat oFeat = (Feat) o;
                return id == oFeat.id;
            }
            public int compareTo(Object o) {
                Feat oFeat = (Feat) o;
                if (id < oFeat.id) {
                    return -1;
                } else if (id == oFeat.id) {
                    return 0;
                } else {
                    return 1;
                }
            }
            public void process() {
                wordList.process();
            }
        } //Feat class
        
        FeatList(CutOff featCf, CutOff wordCf) {
            this.featCf = featCf;
            this.wordCf = wordCf;
        }
        void process() {
            Iterator it;
            Feat unknF = new Feat(UNKNOWN, 0);
            LinkedList nameList = new LinkedList(map.keySet());
            it = nameList.iterator();
            while (it.hasNext()) {
                String name = (String) it.next();
                Feat f = (Feat) map.get(name);
                if (f.cnt < featCf.main) {
                    unknF.cnt += f.cnt;
                    unknF.wordList.addAll(f.wordList);
                    map.remove(name);
                }
            }
            if (unknF.cnt < featCf.unkn) {
                //find rarest feature
                it = map.keySet().iterator();
                Feat rareFeat = null;
                while (it.hasNext()) {
                    String name = (String) it.next();
                    Feat f = (Feat) map.get(name);
                    if (rareFeat == null || f.cnt < rareFeat.cnt) {
                        rareFeat = f;
                    }
                }
                if (rareFeat != null) {
                    unknF.cnt += rareFeat.cnt;
                    unknF.wordList.addAll(rareFeat.wordList);
                    map.remove(rareFeat.name);
                }
            }
            map.put(UNKNOWN, unknF); 
          
            //renumber
            it = map.keySet().iterator();
            LinkedList featList = new LinkedList();
            SortedMap sm = new TreeMap();
            while (it.hasNext()) {
                Object feat =  it.next();
                sm.put(map.get(feat), feat);
            }
            it = sm.keySet().iterator();
            int id = 0;
            while (it.hasNext()) {
                Feat feat = (Feat) it.next();
                feat.id = id++;
            }
             
            
            //process word lists
            it = map.keySet().iterator();
            while (it.hasNext()) {
                String name = (String) it.next();
                Feat feat = (Feat) map.get(name);
                feat.process();     
            }
        }
        void occurred(Record rec) {
            String key;
            key = removeMiddle(rec.feat, '_');
            if (map.containsKey(key)) {
                Feat feat =  (Feat) map.get(key);
                feat.occurred(rec);
            } else {
                Feat feat = new Feat(key, nextId++);
                feat.occurred(rec);
                map.put(key, feat);
            }
        }

        private Feat getFeat(String name) throws NoSuchFieldException{
            name = removeMiddle(name, '_');
            if (!map.containsKey(name)) {
                throw new NoSuchFieldException("Unknown Feat '" + name + "'");
            }
            return (Feat) map.get(name);
        }
        int getId(String name) throws NoSuchFieldException {
            name = removeMiddle(name, '_');
            if (!map.containsKey(name)) {
                return getFeat(UNKNOWN).id;    
            } else {
                return getFeat(name).id;
            }
        }

        int getCnt(String name) throws NoSuchFieldException {
            return getFeat(name).cnt;
        }
       
        int size() {
            return map.size();
        }
        WordList getWordList(String name) throws NoSuchFieldException {
            name = removeMiddle(name, '_');
            if (!map.containsKey(name)) {
                return getFeat(UNKNOWN).wordList;
            } else {
                return getFeat(name).wordList;
            }
        }
        
        LinkedList getNamesSorted() {
            Iterator it = map.keySet().iterator();
            LinkedList featList = new LinkedList();
            SortedMap sm = new TreeMap();
            while (it.hasNext()) {
                Object feat =  it.next();
                sm.put(map.get(feat), feat);
            }
            it = sm.keySet().iterator();
            while (it.hasNext()) {
                featList.addLast(sm.get(it.next()));
            }
            return featList;            
        }
    }

    public class WordList {
        CutOff cf;
        Map map = new TreeMap();
        int nextId = 1;
        public class Word implements Comparable {
            String name;
            int id;
            int cnt = 0;
            boolean forceInclude = false;
            Word(String name, int id) {
                this.name = name;
                this.id = id;
            }
            void occurred() {
                cnt++;
            }
            public boolean equals(Object o) {
                if (! (o instanceof Word)) {
                    return false;
                }
                Word oWord = (Word) o;
                return id == oWord.id;
            }
            public int compareTo(Object o) {
                Word oWord = (Word) o;
                if (id < oWord.id) {
                    return -1;
                } else if (id == oWord.id) {
                    return 0;
                } else {
                    return 1;
                }
            }

            //gurantee that it is included in vocabulary even if it is not frequent enought
            public void forceInclude() {
                forceInclude = true;
            }

        } //Word class
        
        WordList(CutOff cf) {
            this.cf = cf;
        }

        void addAll(WordList wl) {
            Iterator it = wl.map.keySet().iterator();
            while (it.hasNext()) {
                String name = (String) it.next();
                Word w = (Word) wl.map.get(name);
                if (map.containsKey(name)) {
                    Word wHere = (Word) map.get(name);
                    wHere.cnt += w.cnt;    
                } else {
                    Word wHere = new Word(name, nextId++);
                    wHere.cnt = w.cnt;
                    map.put(name, wHere);
                }
            }
        }
        
        void process() {
            Word unknW = new Word(UNKNOWN, 0);
            LinkedList nameList = new LinkedList(map.keySet());
            Iterator it = nameList.iterator();
            while (it.hasNext()) {
                String name = (String) it.next();
                Word w = (Word) map.get(name);
                if (w.cnt < cf.main && !w.forceInclude) {
                    unknW.cnt += w.cnt;
                    map.remove(name);
                }
            }
            if (unknW.cnt < cf.unkn) {
                //find rarest word
                it = map.keySet().iterator();
                Word rareWord = null;
                while (it.hasNext()) {
                    String name = (String) it.next();
                    Word w = (Word) map.get(name);
                    if ( (rareWord == null || w.cnt < rareWord.cnt) &&  !w.forceInclude) {
                        rareWord = w;
                    }
                }
                if (rareWord != null) {
                    unknW.cnt += rareWord.cnt;
                    map.remove(rareWord.name);
                }
            }
            map.put(UNKNOWN, unknW);

            //renumber
            it = map.keySet().iterator();
            LinkedList wordList = new LinkedList();
            SortedMap sm = new TreeMap();
            while (it.hasNext()) {
                Object word =  it.next();
                sm.put(map.get(word), word);
            }
            it = sm.keySet().iterator();
            int id = 0;
            while (it.hasNext()) {
                Word w = (Word) it.next();
                w.id = id++;
            }
 
        }
        void occurred(String s) {
            String key;
            key = s;
            if (map.containsKey(key)) {
                Word word =  (Word) map.get(key);
                word.occurred();
            } else {
                Word word = new Word(key, nextId++);
                word.occurred();
                map.put(key, word);
            }
        }
        
        void forceInclude(String name) throws NoSuchFieldException {
             if (!map.containsKey(name)) {
                throw new NoSuchFieldException("Unknown Word '" + name + "'");
            }
            ((Word) map.get(name)).forceInclude();
        }

        private Word getWord(String name) throws NoSuchFieldException {
            if (!map.containsKey(name)) {
                throw new NoSuchFieldException("Unknown Word '" + name + "'");
            }
            return (Word) map.get(name);
        }
        int getId(String name) throws NoSuchFieldException {
            if (!map.containsKey(name)) {
                return getWord(UNKNOWN).id;        
            } else {
                return getWord(name).id;
            }
        }
        
        int getCnt(String name) throws NoSuchFieldException {
            return getWord(name).cnt;    
        }
       
        int size() {
            return map.size();
        }
        
        LinkedList getNamesSorted() {
            Iterator it = map.keySet().iterator();
            LinkedList wordList = new LinkedList();
            SortedMap sm = new TreeMap();
            while (it.hasNext()) {
                Object word =  it.next();
                sm.put(map.get(word), word);
            }
            it = sm.keySet().iterator();
            while (it.hasNext()) {
                wordList.addLast(sm.get(it.next()));
            }
            return wordList;            
        }    
    } //WordList
    
    //------------------------------ Variables ------------------------------------
    //cutoffs 
    private CutOff 
        wordIn = new CutOff(), 
        wordOut = new CutOff(),
        lemma = new CutOff(), 
        featIn = new CutOff(), 
        featOut = new CutOff();

    private String rootLabel = ROOT_DEP;
    private static final int MAX_FILES = 20;

    private boolean toLowerCase = true;

    private String specFile =  null, hFile = null,  trainFileIn = null, trainFileOut = null,
        testFilesIn[] = new String[MAX_FILES], testFilesOut[] =  new String[MAX_FILES];

    //if set unknown features are 'replaced' by closest ones
    private boolean searchFeat = false;

    private WordList lemmaList;
    private FixedList cposList, relList, elemFeatList, roleLists[];
    private PosList posList;
    private SenseList senseLists[];

    //------------------------------ Methods ----------------------------------------
    public static void printUsageExit() {
        System.err.println("Preprocessor for CoNLL format files: adds numeric codes to fields");
        System.err.println("Usage:\t" +
          "java PreprocessData [-root_label label] [-word_in cf ucf] "
          + "[-word_out cf ucf] [-feat_in cf ucf] [-feat_out cf ucf] [-lemma cf ucf] "
          + "[-no_word_normalization] spec_file h_file (file_in file_out)+");
        System.exit(0);
    }


    void processFile(String inFile, String outFile) throws NoSuchFieldException, IOException {
        BufferedReader in = new BufferedReader(new FileReader(inFile));
        BufferedWriter out = new BufferedWriter(new FileWriter(outFile));
        String line;
        int lineNo = 0;
        while((line = in.readLine()) != null) {
            lineNo++;
            line = line.trim();
            if (line.equals("")) {
                out.write("\n");
                continue;
            }
            Record rec =  Record.parseRecord(line);            
            String s =  rec.idx + "\t" + rec.word + "\t";
            
            
            FeatList flOut = posList.getOutFeatList(rec.pos), flIn = posList.getInFeatList(rec.pos);
            WordList wlOut = flOut.getWordList(rec.feat), wlIn = flIn.getWordList(rec.feat);
            String elemFeatS = "";
            Iterator elemFeatIt = rec.elemFeatures.iterator();
            while (elemFeatIt.hasNext()) {
                String elemName = (String) elemFeatIt.next();
                if (!elemFeatList.contains(elemName)) {
                    System.err.println("Warning: unknown elementary feature '"+ elemName + "' in file '" + inFile + "', ignoring");
                    continue;
                }
                if (!elemFeatS.equals("")) {
                    elemFeatS += "|";
                }
                elemFeatS += elemFeatList.getId(elemName);
            }
            if (elemFeatS.length() == 0) {
                elemFeatS = "_";
            } 
            
            String w = rec.word;
            if (toLowerCase) {
                w = w.toLowerCase();
            }
            s += wlIn.getId(w) + "\t";
            s += wlOut.getId(w) + "\t";
            
            s += rec.lemma + "\t" + lemmaList.getId(rec.lemma) + "\t";
            s += rec.cpos + "\t" + cposList.getId(rec.cpos) + "\t";
            s += rec.pos + "\t" + posList.getId(rec.pos) + "\t";
            
            s += rec.feat + "\t";
            
            s += flIn.getId(rec.feat) + "\t";
            s += flOut.getId(rec.feat) + "\t";
            s += elemFeatS + "\t";
            s += rec.bank + "\t";
                
    
            if (rec.head >= 0) {
                int recId = -1;
                String rel =  rec.rel;
                try {
                    recId = relList.getId(rec.rel);
                } catch (NoSuchFieldException e) {
                    System.err.println("Warning - unexpected token: " + e.getMessage());
                    //e.printStackTrace();
                    StringTokenizer stPipe = new StringTokenizer(rec.rel, "|");
                    String repairedRel = null;
                    if  (stPipe.hasMoreTokens()) {
                        repairedRel = stPipe.nextToken();
                    } 
                    if (repairedRel == null || repairedRel.equals(rec.rel)) {
                        throw new NoSuchFieldException("==>Can't resolve problem with unexpected token, stopping");
                    }
                    
                    System.err.println("==> Trying '" + repairedRel +  "' instead of '" + rec.rel + "'");
                    recId = relList.getId(repairedRel);
                    System.err.println("==> Don't trust acurracy reported by idp on this file anymore");
                    System.err.println("==> Use scripts/ext2conll and eval07.pl instead as explained in README");
                    System.err.println("==> If it is a validation set - ignore this warning");
                    rel = repairedRel;
                }
                s += "\t" + rec.head + "\t" + rel + "\t" + recId;
                
                int senseId;
                if (rec.sense.equals("_")) {
                    senseId = -1;
                } else {
                    if (rec.bank < 0) {
                        System.err.println("Warning: bank < 0 with sense = '" + rec.sense + "', changing to sense = '_'");
                        rec.sense = "_";
                        senseId = -1;    
                    } else  {
                        try {
                            senseId = senseLists[rec.bank].getId(rec.lemma, rec.sense);
                        }  catch (NoSuchFieldException e) {
                            System.err.println("Warning: unknown predicate " + rec.sense + ", removing it; Do not believe reported accuracy anymore!!!");
                            senseId  =-1;
                        }
                    }
                }

                s += "\t" + rec.sense + "\t" +  senseId;
                Iterator argsIt = rec.args.iterator();
                while (argsIt.hasNext()) {
                    if (rec.bank < 0) {
                        System.err.println("Warning: a token with bank < 0 has non-empty list of arguments, removing it");
                        break;
                    } else if (senseId < 0) {
                         System.err.println("Warning: now removing arguments");
                         break;
                    } else {
                        PosRolePair posRolePair = (PosRolePair) argsIt.next();
			try{
				int roleId =  roleLists[rec.bank].getId(posRolePair.role);
				s += "\t" + posRolePair.a + "\t" + posRolePair.role + "\t" + roleId;
			}catch(NoSuchFieldException e ){
				System.err.println("Warning: removing unkown argument: "+posRolePair.role);
			}
                    }
                }
            }
            s += "\n";    
            out.write(s);
        }        
        in.close();
        out.close(); 
    }

    public class SenseList {
        TreeMap lemma2Senses = new TreeMap();
        
        int size() {
            return lemma2Senses.size();
        }

        void occurred(String lemma, String sense) {
            if (!lemma2Senses.containsKey(lemma)) {
                lemma2Senses.put(lemma, new FixedList());
            }
            FixedList senses = (FixedList) lemma2Senses.get(lemma);
            senses.occurred(sense);
        }
        
        
        int getId(String lemma, String sense) throws NoSuchFieldException {
            if (!lemma2Senses.containsKey(lemma)) {
                throw new NoSuchFieldException("Unknown lemma, no sense registered '" + lemma + "'");
            }
            FixedList senses = (FixedList) lemma2Senses.get(lemma);
            return senses.getId(sense);
        }

        boolean contains(String name) {
            return lemma2Senses.containsKey(name);
        }
        
        LinkedList getNamesSorted() {
            Iterator it = lemma2Senses.keySet().iterator();
            LinkedList list = new LinkedList();
            while (it.hasNext()) {
                list.addLast(it.next());
            }
            return list;            
        } 

        LinkedList getSensesSorted(String lemma) throws NoSuchFieldException {
             if (!lemma2Senses.containsKey(lemma)) {
                throw new NoSuchFieldException("Unknown lemma, no sense registered '" + lemma + "'");
            }
            FixedList senses = (FixedList) lemma2Senses.get(lemma);
            return senses.getNamesSorted();           
        }


    }

    void prepareStructure(String inFile) throws IOException {
        lemmaList = new WordList(lemma);
        cposList = new FixedList();
        relList = new FixedList(rootLabel);
        posList = new PosList();     
        elemFeatList = new FixedList();
        
        senseLists = new SenseList[BANK_NUM];
        roleLists = new FixedList[BANK_NUM];
        for (int bank = 0; bank < BANK_NUM; bank++) {
            senseLists[bank] = new SenseList();
            roleLists[bank] = new FixedList();
        }
        
        BufferedReader in = new BufferedReader(new FileReader(inFile));

        String line;
        while((line = in.readLine()) != null) {
            line = line.trim();
            if (line.equals("")) {
                continue;
            }
            Record rec =  Record.parseRecord(line);            
            lemmaList.occurred(rec.lemma);
            cposList.occurred(rec.cpos);
            relList.occurred(rec.rel);
            posList.occurred(rec);
            if (!rec.sense.equals("_")) {
                if (rec.bank < 0 || rec.bank >= BANK_NUM) {
                    System.err.println("Warning: BANK not in range (" + rec.bank + ") for a token with SENSE = " + rec.sense + " (LEMMA = '" + rec.lemma + "')");
                    rec.sense = "_";
                } else {
                    senseLists[rec.bank].occurred(rec.lemma, rec.sense);
                }

            }

            Iterator elListIt = rec.elemFeatures.iterator();
            while (elListIt.hasNext()) {
                elemFeatList.occurred((String) elListIt.next()); 
            }
            
            Iterator argsIt = rec.args.iterator();
            while (argsIt.hasNext()) {
                if (rec.bank < 0) {
                    System.err.println("Warning: BANK < 0 for non empty list of arguments  (LEMMA = '" + rec.lemma + "')");
                    break;
                }
                roleLists[rec.bank].occurred( ((PosRolePair) argsIt.next()).role);
                //we must ensure that this lemma is included as a separate word
                try {
                    lemmaList.forceInclude(rec.lemma);
                } catch (NoSuchFieldException e) {
                    throw new IOException(e);
                }
            }


        }
        in.close();
        lemmaList.process();
        posList.process();
    }

    String paramStrings() {
        String s = "";
        s += "# to_lower_case (" + toLowerCase + ")\n";
        s += "# root_label (" + rootLabel + ") \n";
        s += "# word_in (" + wordIn + ") ";
        s += "word_out (" + wordOut + ") ";
        s += "feat_in (" + featIn + ") ";
        s += "feat_out (" + featOut + ") ";
        s += "lemma (" + lemma + ") \n";
        s += "# search_feat = " + searchFeat + "\n";
        s += "# spec_file = '" + specFile + "'\n";
        s += "# h_file = '" + hFile + "'\n";
        s += "# train_file = '" + trainFileIn + "'/'" + trainFileOut + "'\n";
        for (int idx = 0; idx < testFilesOut.length; idx++) {
            if (testFilesIn[idx] == null) {
                break;
            }
            s += "# test_files["+ idx + "] = '" + testFilesIn[idx] + "'/'" + testFilesOut[idx] + "'\n";
        }    
        return s;
    }

    void writeSpec(String specFile) throws IOException, NoSuchFieldException {
        BufferedWriter out = new BufferedWriter(new FileWriter(specFile));
        out.write(paramStrings()); out.flush();

        out.write("\n# ------ CPOS ---------\n");
        LinkedList cposNames = cposList.getNamesSorted();
        out.write("# Size\n" +  cposNames.size() + "\n"); out.flush();
        Iterator it = cposNames.iterator(); 
        int idx = 0;
        out.write("# Contents\n"); out.flush();
        while (it.hasNext()) {
           String name = (String) it.next();
           out.write("# " + idx + "\t" + name + "\n"); out.flush();
           idx++;
        }

        out.write("\n# ------ POS ---------\n"); out.flush();
        LinkedList posNames = posList.getNamesSorted();
        out.write("# Size\n" +  posNames.size() + "\n"); out.flush();
        it = posNames.iterator();
        idx = 0;
        out.write("# Contents\n"); out.flush();
        while (it.hasNext()) {
           String name = (String) it.next();
           out.write("# " + idx + "\t" + name + "\n"); out.flush();
           idx++;
        }

        out.write("\n# ------ FEAT IN ---------\n"); out.flush();
        it = posNames.iterator();

        String header = "", comment = "";
        int posIdx = 0;
        while (it.hasNext()) {
            String posName = (String) it.next();
            FeatList featList = posList.getInFeatList(posName);
            LinkedList featNames = featList.getNamesSorted();
            header +=  featNames.size() + " ";
            Iterator fIt = featNames.iterator();
            int featIdx =  0;
            while (fIt.hasNext()) {
                String featName = (String) fIt.next();
                int cnt = featList.getCnt(featName);
                comment += "# " + posIdx + "\t" + posName + "\t" + featIdx + "\t" +  featName + " (" + cnt + ")\n";  
                featIdx++;
            }
            posIdx++;
        }
        out.write(header + "\n"); out.flush();
        out.write(comment); out.flush();


        out.write("\n# ------ WORD IN ---------\n"); out.flush();
        it = posNames.iterator();

        header = "";  comment = "";
        StringBuffer commentB = new StringBuffer();
        posIdx = 0;
        int i = 0;
        while (it.hasNext()) {
            String posName = (String) it.next();
            FeatList featList = posList.getInFeatList(posName);
            LinkedList featNames = featList.getNamesSorted();
//            header +=  featNames.size() + " ";
            Iterator fIt = featNames.iterator();
            int featIdx =  0;
            while (fIt.hasNext()) {
                String featName = (String) fIt.next();
                WordList wordList = featList.getWordList(featName);
                LinkedList wordNames = wordList.getNamesSorted();
                header += wordNames.size() + " ";
                Iterator wIt = wordNames.iterator();
                int wIdx = 0;
                while (wIt.hasNext()) {
                    String wordName = (String) wIt.next();
                    int cnt = wordList.getCnt(wordName);
                    commentB.append("# ");
                    commentB.append(posIdx);
                    commentB.append("\t");
                    commentB.append(posName);
                    commentB.append("\t");
                    commentB.append(featIdx);
                    commentB.append("\t");
                    commentB.append(featName);
                    commentB.append("\t");
                    commentB.append(wIdx);
                    commentB.append("\t");
                    commentB.append(wordName);
                    commentB.append("\t(");
                    commentB.append(cnt);
                    commentB.append(")\n"); 
                    
                    wIdx++;
                    i++;
                }
                System.out.flush();
                featIdx++;
            }
            System.out.flush();
            header += "\n";
            posIdx++;
        }
        out.write(header); out.flush();
        out.write(commentB.toString()); out.flush();
        

        out.write("\n# ------ FEAT OUT ---------\n"); out.flush();
        it = posNames.iterator();

        header = ""; comment = "";
        posIdx = 0;
        while (it.hasNext()) {
            String posName = (String) it.next();
            FeatList featList = posList.getOutFeatList(posName);
            LinkedList featNames = featList.getNamesSorted();
            header +=  featNames.size() + " ";
            Iterator fIt = featNames.iterator();
            int featIdx =  0;
            while (fIt.hasNext()) {
                String featName = (String) fIt.next();
                int cnt = featList.getCnt(featName);
                comment += "# " + posIdx + "\t" + posName + "\t" + featIdx + "\t" +  featName + " (" + cnt + ")\n";  
                featIdx++;
            }
            posIdx++;
        }
        out.write(header + "\n"); out.flush();
        out.write(comment); out.flush();


        out.write("\n# ------ WORD OUT ---------\n"); out.flush();
        it = posNames.iterator();

        header = "";  comment = "";
        commentB = new StringBuffer();
        posIdx = 0;
        i  = 0;
        while (it.hasNext()) {
            String posName = (String) it.next();
            FeatList featList = posList.getOutFeatList(posName);
            LinkedList featNames = featList.getNamesSorted();
            Iterator fIt = featNames.iterator();
            int featIdx =  0;
            while (fIt.hasNext()) {
                String featName = (String) fIt.next();
                WordList wordList = featList.getWordList(featName);
                LinkedList wordNames = wordList.getNamesSorted();
                header += wordNames.size() + " ";
                Iterator wIt = wordNames.iterator();
                int wIdx = 0;
                while (wIt.hasNext()) {
                    String wordName = (String) wIt.next();
                    int cnt = wordList.getCnt(wordName);
                    commentB.append("# ");
                    commentB.append(posIdx);
                    commentB.append("\t");
                    commentB.append(posName);
                    commentB.append("\t");
                    commentB.append(featIdx);
                    commentB.append("\t");
                    commentB.append(featName);
                    commentB.append("\t");
                    commentB.append(wIdx);
                    commentB.append("\t");
                    commentB.append(wordName);
                    commentB.append("\t(");
                    commentB.append(cnt);
                    commentB.append(")\n"); 
                    
                    wIdx++;
                    i++;
                }
                featIdx++;
            }
            header += "\n";
            posIdx++;
        }
        out.write(header); out.flush();
        out.write(commentB.toString()); out.flush();
 
        out.write("\n# ------ LEMMA ---------\n"); out.flush();
        LinkedList lemmaNames = lemmaList.getNamesSorted();
        out.write("# Size\n" +  lemmaNames.size() + "\n"); out.flush();
        Iterator wIt = lemmaNames.iterator();
        int lIdx = 0;
        while (wIt.hasNext()) {
            String lemmaName = (String) wIt.next();
            int cnt = lemmaList.getCnt(lemmaName);
            out.write("# " + lIdx + "\t" + lemmaName + "\t"
                + " (" + cnt + ")\n");
            out.flush();
            lIdx++;
        }
 

        out.write("\n# ------ REL ---------\n"); out.flush();
        LinkedList relNames = relList.getNamesSorted();
        out.write("# Size (without '" + rootLabel + "')\n" +  relNames.size() + "\n"); out.flush();
        int rootLabelId = relList.getId(rootLabel);
        out.write("# Root label information\n" + rootLabelId + "\t" + rootLabel + "\n"); out.flush();
        it = relNames.iterator();
        idx = 0;
        out.write("# Contents\n"); out.flush();
        while (it.hasNext()) {
           String name = (String) it.next();
           out.write(idx + "\t" + name + "\n"); out.flush();
           idx++;
        }
        
        out.write("\n# ------ ELEM_FEAT ---------\n");
        LinkedList elFeatNames = elemFeatList.getNamesSorted();
        out.write("# Size\n" +  elFeatNames.size() + "\n"); out.flush();
        it = elFeatNames.iterator(); 
        idx = 0;
        out.write("# Contents\n"); out.flush();
        while (it.hasNext()) {
           String name = (String) it.next();
           out.write("# " + idx + "\t" + name + "\n"); out.flush();
           idx++;
        }

        for (int bank = 0; bank < BANK_NUM; bank++) {
            out.write("\n# ------ SENSE_LIST_" + bank + " ---------\n"); out.flush();
            LinkedList predicateLemmas = senseLists[bank].getNamesSorted();
            out.write("# Number of different lemmas which have associated predicates\n" + senseLists[bank].size() + "\n"); out.flush();
            it = predicateLemmas.iterator();
            while (it.hasNext()) {
                out.write("# Lemma_Id, Lemma and number of senses for it:\n"); out.flush();
                String lemma = (String) it.next();
                int lemmaId = lemmaList.getId(lemma);
                LinkedList senses = senseLists[bank].getSensesSorted(lemma);
                out.write(lemmaId + "\t" + lemma + "\t" + senses.size() + "\n"); out.flush();
                out.write("# Senses: Sense_Id, Sense\n");
                Iterator sensesIt = senses.iterator();
                int senseId = 0;
                while (sensesIt.hasNext()) {
                    String sense = (String) sensesIt.next();
                    out.write(senseId + "\t" + sense + "\n"); out.flush();
                    senseId++;
                }
            }
             

            out.write("\n# ------ ARG_LIST_" + bank + " ---------\n"); out.flush();
            LinkedList roleNames =   roleLists[bank].getNamesSorted();
            out.write("# Size\n" +  roleLists[bank].size() + "\n"); out.flush();
            it = roleNames.iterator();
            idx = 0;
            out.write("# Contents\n"); out.flush();
            while (it.hasNext()) {
               String name = (String) it.next();
               out.write(idx + "\t" + name + "\n"); out.flush();
               idx++;
            }
        }     

        
        out.close(); 
    }

    boolean processParams(String argv[]) {
        if (argv.length < 4) {
            return false;
        }
        for (int idx = 0; idx < argv.length; ) {
            if (argv[idx].equals("-word_in")) {
                idx++;
                wordIn.main = Integer.parseInt(argv[idx++]);
                wordIn.unkn = Integer.parseInt(argv[idx++]);
            } else if (argv[idx].equals("-word_out")) {
                idx++;
                wordOut.main = Integer.parseInt(argv[idx++]);
                wordOut.unkn = Integer.parseInt(argv[idx++]);
            } else if (argv[idx].equals("-feat_in")) {
                idx++;
                featIn.main = Integer.parseInt(argv[idx++]);
                featIn.unkn = Integer.parseInt(argv[idx++]);
            } else if (argv[idx].equals("-feat_out")) {
                idx++;
                featOut.main = Integer.parseInt(argv[idx++]);
                featOut.unkn = Integer.parseInt(argv[idx++]);
            } else if (argv[idx].equals("-lemma")) {
                idx++;
                lemma.main = Integer.parseInt(argv[idx++]);
                lemma.unkn = Integer.parseInt(argv[idx++]);
            } else if (argv[idx].equals("-root_label")) {
                idx++;
                rootLabel = argv[idx++];
            }  else if (argv[idx].equals("-search_feat")) {
                idx++;
                searchFeat = true;
            }  else if (argv[idx].equals("-no_word_normalization")) {
                idx++;    
                toLowerCase = false;
            } else {
                specFile = argv[idx++];
                hFile = argv[idx++];
                trainFileIn = argv[idx++];        
                trainFileOut = argv[idx++];
                int testIdx = 0;
                while (idx < argv.length) {
                    testFilesIn[testIdx] = argv[idx++];
                    testFilesOut[testIdx] = argv[idx++];
                    testIdx++;
                }
                if (idx != argv.length) {
                    return false;
                }
            }
        }
        if (specFile == null) {
            System.err.println("Error: spec_file is not defined");
            return false;
        }
        if (hFile == null) {
            System.err.println("Error: h_file is not defined");
        }
        if (trainFileOut == null || trainFileIn == null) {
            System.err.println("Error: no files to process are given");
            return false;
        }
        
        System.out.println("Parameters:\n---------\n" + paramStrings() + "---------");
        return true;
    }
    
	/**
	 * Default constructor for PreprocessData
	 */
	public PreprocessData() {
	}


    public static void main(String argv[]) throws Exception {
        try {
            PreprocessData preproc = new PreprocessData();
            if (!preproc.processParams(argv)) {
                System.err.println("Error: Wrong usage format");
                printUsageExit();    
            }
            System.out.println("Preprocessor for CoNLL format files");
            System.out.println("Reading file '" +  preproc.trainFileIn + "' and creating string to code mapping...");
            preproc.prepareStructure(preproc.trainFileIn);
            System.out.println("Writing created specification to '" + preproc.specFile + "'...");
            preproc.writeSpec(preproc.specFile);
            System.out.println("Writing create header file to '" + preproc.hFile + "'...");
            preproc.writeHeaderFile(preproc.hFile);
            System.out.println("==============================================================================");
            System.out.println("Processing file '" + preproc.trainFileIn + "' and writing results to '" + preproc.trainFileOut + "'...");
            System.out.println("==============================================================================");
            preproc.processFile(preproc.trainFileIn, preproc.trainFileOut);
            for (int idx = 0; idx < preproc.testFilesOut.length; idx++) {
                if (preproc.testFilesOut[idx] == null) {
                    break;
                }
                System.out.println("==============================================================================");
                System.out.println("Processing file '" + preproc.testFilesIn[idx] + "' and writing results to '" + preproc.testFilesOut[idx] + "'...");
                System.out.println("==============================================================================");
                preproc.processFile(preproc.testFilesIn[idx], preproc.testFilesOut[idx]);
            }
        } catch (IOException e) {
            System.err.println("Input-Output error: " + e.getMessage() + ", stopped");
            e.printStackTrace();
            System.err.println("Preprocessing FAILED");
            System.exit(1);
        } catch (NoSuchFieldException e) {
            System.err.println("Unexpected token: " + e.getMessage() + ", stopped");
            e.printStackTrace();
            System.err.println("Preprocessing FAILED");
            System.exit(2);
        }
    }
    
    String headerPreambule() {
        String def =  "_" + hFile.toUpperCase();
        return 
                  "#ifndef _IDP_IO_SPEC_H \n"
                + "#define _IDP_IO_SPEC_H \n\n\n"
                + "//NB: *_INP_* are assumed to be at least as large as *_OUT_*\n\n"; 
    }
    

    void writeHeaderFile(String hFile) throws IOException, NoSuchFieldException {
        /** @todo: remove maxIn, maxOut  */
        BufferedWriter out = new BufferedWriter(new FileWriter(hFile));
        
        out.write(headerPreambule()); 

        String def = "#define ";       
        
        class Max{
            int max = 0;
            void update(int x) {
                if (x > max) {
                    max = x;
                }
            }
            public String toString() {
                return "" + max;
            }
            public int intValue() {
                return max;
            }

        }
        Max maxOut = new Max(), maxIn = new Max();
             
        out.write(def + "MAX_DEPREL_SIZE " + relList.size() + "\n\n"); 
        maxIn.update(relList.size());
        maxOut.update(relList.size());

        out.write(def + "MAX_POS_INP_SIZE " + posList.size() + "\n");
        maxIn.update(posList.size());
        out.write(def + "MAX_POS_OUT_SIZE " + posList.size() + "\n");
        maxOut.update(posList.size());
        

        LinkedList posNames = posList.getNamesSorted();
        Iterator it = posNames.iterator();

        int maxInFeatNum = 1;
        while (it.hasNext()) {
            String posName = (String) it.next();
            int featNum = posList.getInFeatList(posName).size();
            if (featNum > maxInFeatNum) {
                maxInFeatNum = featNum;
            }
        }
        out.write(def + "MAX_FEAT_INP_SIZE " + maxInFeatNum + "\n");
        maxIn.update(maxInFeatNum);
        
        it = posNames.iterator();
        int maxOutFeatNum = 1;
        while (it.hasNext()) {
            String posName = (String) it.next();
            int featNum = posList.getOutFeatList(posName).size();
            if (featNum > maxOutFeatNum) {
                maxOutFeatNum = featNum;
            }
        }
        out.write(def + "MAX_FEAT_OUT_SIZE " + maxOutFeatNum + "\n");
        maxOut.update(maxOutFeatNum);

        it = posNames.iterator();
        int maxInWordNum = 1;
        int posIdx = 0;
        while (it.hasNext()) {
            FeatList featList = posList.getInFeatList((String) it.next());
            Iterator fIt = featList.getNamesSorted().iterator();
            while (fIt.hasNext()) {
                String featName = (String) fIt.next();
                WordList wordList = featList.getWordList(featName);
                int wordNum = wordList.size();
                if (wordNum > maxInWordNum) {
                    maxInWordNum = wordNum;
                }
            }
        }
        out.write(def + "MAX_WORD_INP_SIZE " + maxInWordNum + "\n");
        maxIn.update(maxInWordNum);

        it = posNames.iterator();
        int maxOutWordNum = 1;
        posIdx = 0;
        while (it.hasNext()) {
            FeatList featList = posList.getOutFeatList((String) it.next());
            Iterator fIt = featList.getNamesSorted().iterator();
            while (fIt.hasNext()) {
                String featName = (String) fIt.next();
                WordList wordList = featList.getWordList(featName);
                int wordNum = wordList.size();
                if (wordNum > maxOutWordNum) {
                    maxOutWordNum = wordNum;
                }
            }
        }
        out.write(def + "MAX_WORD_OUT_SIZE " + maxOutWordNum + "\n");
        maxOut.update(maxOutWordNum);
        
        out.write("\n\n//to be used only in inputs\n");
        out.write(def + "MAX_CPOS_SIZE " + cposList.size() + "\n");
        maxIn.update(cposList.size());
        out.write(def + "MAX_LEMMA_SIZE "  + lemmaList.size() +"\n");
        maxIn.update(lemmaList.size());
        out.write(def + "MAX_ELFEAT_SIZE " + elemFeatList.size() + "\n\n");
        maxIn.update(elemFeatList.size());

        maxIn.update(maxOut.intValue());

        out.write("//set MAX_OUT_SIZE to MAX of *_OUT_* and MAX_DEPREL_SIZE\n");
		out.write("#define MAX_OUT_SIZE MAX_DEPREL_SIZE\n");
		out.write("#if MAX_POS_OUT_SIZE > MAX_OUT_SIZE\n");
		out.write("#define MAX_OUT_SIZE MAX_POS_OUT_SIZE \n");
        out.write("#undef MAX_OUT_SIZE\n");
		out.write("#endif\n");
		out.write("#if MAX_WORD_OUT_SIZE > MAX_OUT_SIZE\n");
        out.write("#undef MAX_OUT_SIZE\n");
		out.write("#define MAX_OUT_SIZE MAX_WORD_OUT_SIZE \n");
		out.write("#endif\n");
		out.write("#if MAX_FEAT_OUT_SIZE > MAX_OUT_SIZE \n");
        out.write("#undef MAX_OUT_SIZE\n");
		out.write("#define MAX_OUT_SIZE MAX_FEAT_OUT_SIZE \n");
		out.write("#endif\n\n");


        out.write("//maximum length of a sentence (hardcoded, not computed)\n");
        out.write("#define MAX_SENT_LEN 400\n\n");

        out.write("//maximum number of arguments (hardcoded, not computed)\n");
        out.write("#define MAX_ARG_NUM 45\n\n");

        int maxRoleSize = 0;
        for (int bank = 0; bank < BANK_NUM; bank++) {
            if (roleLists[bank].size() > maxRoleSize) {
                maxRoleSize =  roleLists[bank].size();
            }
        }
        out.write("//maximum number of different roles (per treebank)\n");
        out.write("#define MAX_ROLE_SIZE " + maxRoleSize +  "\n\n");



        int maxSenseSize = 0;
        for (int bank = 0; bank < BANK_NUM; bank++) {
            LinkedList predicateLemmas = senseLists[bank].getNamesSorted();
            it = predicateLemmas.iterator();
            while (it.hasNext()) {
                String lemma = (String) it.next();
                int senseSize  = senseLists[bank].getSensesSorted(lemma).size();
                if (senseSize > maxSenseSize) {
                    maxSenseSize = senseSize;
                }
            }
        }     

        out.write("//maximum number of senses (per lemma per treebank)\n");        
        out.write("#define MAX_SENSE_SIZE " + maxSenseSize + "\n\n");

        out.write("#endif\n");

        out.close(); 
    }
}


