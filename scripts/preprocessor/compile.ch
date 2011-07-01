rm -Rf classes/*
javac -d classes -target 1.5 idpparser/PreprocessData.java 
cd classes
jar cvfm idp_preprocessor.jar ../manifest  idpparser
cd ..
mv classes/idp_preprocessor.jar .
java -jar idp_preprocessor.jar 
