#include "include/ontology.h"
#include "include/relation.h"
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct ontology{
    char * title;
    char * baselanguage;
    GHashTable * suportedLanguages; /* Set<char *> */
    GHashTable * noteSections; /* Set<char*> */
    GHashTable * concepts; /* Map<char*, Concept>*/
    GHashTable * relations; /* Map< Relation, Set<Relation> >*/
} *Ontology;

void setBaseLanguage(Ontology saurus, const char * lang){
    saurus->baselanguage = strdup(lang);
}

void addLanguage(Ontology saurus, const char* lang){
    char * cpy = strdup(lang);
    g_hash_table_insert(saurus->suportedLanguages, cpy, cpy);
}

void addNoteSection(Ontology saurus, const char* notesection){
    char * cpy = strdup(notesection);
    g_hash_table_insert(saurus->noteSections, cpy, cpy);
}

void addTitle(Ontology saurus, const char* title){
    saurus->title = strdup(title);
}

Ontology mkOntology(){
    Ontology ths = (Ontology)malloc(sizeof(struct ontology));
    ths->concepts = g_hash_table_new_full(
        g_str_hash,
        g_str_equal,
        free,
        unmkConcept);
    ths->relations = g_hash_table_new_full(
        hashRelation,
        equalRelation,
        unmkRelation,
        g_hash_table_destroy
    );

    ths->suportedLanguages = g_hash_table_new_full(
        g_str_hash,
        g_str_equal,
        free,
        NULL
    );

    ths->noteSections = g_hash_table_new_full(
        g_str_hash,
        g_str_equal,
        free,
        NULL
    );
    ths->baselanguage = NULL;
    ths->title = NULL;
    return ths;
}

/*
    Identifica os vértices (conceitos) que são a origem de arestas (relações) do tipo "edge" e não têm
    uma aresta desse tipo a incidir neles.

    Estes vértices são os candidatos para serem a raiz de árvores.
*/
GList * findRelationalStartingPoints( Ontology saurus, Relation edge ){

    GList * solution = NULL;
    if( g_hash_table_contains(saurus->relations,edge) ){
        GList * concepts = g_hash_table_get_values(saurus->concepts);
        GList * candidates = NULL;
        GHashTable * conceptHist = g_hash_table_new_full(
            hashConcept,
            equalConcept,
            NULL,
            free);

        for( GList * cur = concepts; cur; cur = cur->next){
            Concept tmpcp = (Concept)cur->data;
            if( isRelatedBy(tmpcp,edge) ){
                candidates = g_list_prepend(candidates, tmpcp);
                fillConceptHistogram(tmpcp, edge, conceptHist);
            }
        }

        for( GList * cur = candidates; cur; cur = cur->next){
            if( !g_hash_table_contains( conceptHist, (Concept)cur->data) )
                solution = g_list_prepend(solution, cur->data);

        }

        g_list_free(candidates);
        g_list_free(concepts);
    }

    return solution;
}

void unmkOntology( Ontology saurus ){
    g_hash_table_destroy(saurus->concepts);
    g_hash_table_destroy(saurus->relations);
    g_hash_table_destroy(saurus->suportedLanguages);
    if( saurus->baselanguage )
        free(saurus->baselanguage);

    if( saurus->title)
        free(saurus->title);

    free(saurus);
}
/*
void addLanguage( Ontology saurus, char * lang ){
    GString slan = g_string_new( lang );
    g_hash_table_insert(saurus->languages, slan, NULL);
}
*/

void showOntology(Ontology saurus){

    FILE *f = fopen("out/html/index.html", "w");
    if(f == NULL) {
        printf("Error opening file %s!\n", "out/html/index.html");
        exit(1);
    }

    //Inicializing
    fprintf(f, "<!DOCTYPE html>\n<html>\n<head>\n\t<title>PL</title>\n");
    fprintf(f, "\t<meta name='viewport' content='width=device-width, initial-scale=1'>\n");
    fprintf(f, "\t<style>\n");
    fprintf(f, "\t\tbody { max-width: 1300px; margin: auto; }\n");
    fprintf(f, "\t\t.colunas { line-height: 200%; margin-left: 60px; -webkit-column-count: 2; -moz-column-count: 2; column-count: 2; }\n");
    fprintf(f, "\t</style>\n</head>\n");

    //Body
    fprintf(f, "<body>\n");
    fprintf(f, "\t<h1><p align='center'><font color='#2874A6'>Ontologias</font></p></h1>\n");
    fprintf(f, "\t<h4>\n");

    int number_of_concepts = g_hash_table_size(saurus->concepts);
    int number_of_relations = g_hash_table_size(saurus->relations);

    fprintf(f, "\t\t<u>Título:</u> %s</br>\n", saurus->title);
    printf("title : %s\n", saurus->title);

    fprintf(f, "\t\t<p align='right'><a href=\"grafo.html\">Ver grafo geral</a></p>\n");

    fprintf(f, "\t\t<u>Linguagem base:</u> %s</br>\n", saurus->baselanguage);
    printf("baselanguage : %s\n", saurus->baselanguage);

    GList* langs = g_hash_table_get_values(saurus->suportedLanguages);

    printf("\tsupported languages");
    fprintf(f, "\t\t<u>Linguagens suportadas:</u>");
    for(GList * cur= langs; cur; cur = cur->next ){
        printf(" %s",(char*)cur->data);
        fprintf(f, " %s", (char *)cur->data);
    }

    printf("\n");
    g_list_free(langs);

    //Create and inicialize relations grafos
    GList *relationSet = g_hash_table_get_keys(saurus->relations);
    for (GList *cur = relationSet; cur; cur = cur->next){
        char *name = getRelationName((Relation)cur->data);

        char grafoFilename[2000];
        sprintf(grafoFilename, "out/grafos/%sgrafo.dot", name);

        FILE *grafo = fopen(grafoFilename, "w");
        if (grafo == NULL){
            printf("Error opening file %s!\n", grafoFilename);
            exit(1);
        }

        fprintf(grafo, "digraph{\n");
        fprintf(grafo, "\trankdir=TB;\n");
    }

    fprintf(f, "\n\t</h4>\n");
    fprintf(f, "\t<h2><p align='center'><font color='#85C1E9'>Conceitos:</font></p></h2>\n");
    fprintf(f, "\t<div class='colunas'>\n");
    fclose(f);

    //Grafo relation file
    FILE *geralGrafo = fopen("out/grafos/grafo.dot", "w");
    if (geralGrafo == NULL){
        printf("Error opening file %s!\n", geralGrafo);
        exit(1);
    }
    fprintf(geralGrafo, "digraph{\n");
    fprintf(geralGrafo, "\trankdir=TB;\n");
    fclose(geralGrafo);

    //Grafo geral HTML
    FILE *grafoHTML = fopen("out/html/grafo.html", "w");
    if (grafoHTML == NULL){
        printf("Error opening file %s!\n", "out/html/grafo.html");
        exit(1);
    }
    fprintf(grafoHTML, "<center><img src='../grafos/grafo.png' alt='Grafo'></center>");
    fclose(grafoHTML);

    //Print concepts
    GList *concepts = g_hash_table_get_values(saurus->concepts);

    for(GList * cur = concepts; cur; cur = cur->next){
        showConcept((Concept)cur->data);
    }

    //Priting grafo in index page
    FILE *fa = fopen("out/html/index.html", "a");
    if(fa == NULL) {
        printf("Error opening file %s!\n", "out/html/index.html");
        exit(1);
    }
    fprintf(fa, "\t</div>\n");
    fprintf(fa, "\t</br><h2><p align='center'><font color='#85C1E9'>Grafos das relações:</font></p></h2>\n");
    fprintf(fa, "\t<div class='colunas'>\n");

    //Grafos Makefile
    FILE *makefile = fopen("out/Makefile", "w");
    if(makefile == NULL) {
        printf("Error opening file %s!\n", "out/Makefile");
        exit(1);
    }
    fprintf(makefile, "all:\n");
    fprintf(makefile, "\tdot -Tpng grafos/grafo.dot > grafos/grafo.png\n");

    //Finish and close grafo files & save them to index.html
    for(GList* cur = relationSet; cur ; cur = cur->next ){
        char *name = getRelationName((Relation)cur->data);

        //Close grafo relation file
        char grafoFilename[2000];
        sprintf(grafoFilename, "out/grafos/%sgrafo.dot", name);
        FILE *grafo = fopen(grafoFilename, "a");
        if(grafo == NULL) {
            printf("Error opening file %s!\n", grafoFilename);
            exit(1);
        }
        fprintf(grafo, "}\n");

        //Grafo relation image page
        char grafoImageFile[2000];
        sprintf(grafoImageFile, "out/html/%sgrafo.html", name);
        FILE *grafoImage = fopen(grafoImageFile, "w");
        if(grafoImage == NULL) {
            printf("Error opening file %s!\n", grafoImageFile);
            exit(1);
        }
        fprintf(grafoImage, "<h1><p align='center'><font color='#2874A6'>%s</font></p></h1></br></br>\n", name);
        fprintf(grafoImage, "<center><img src='../grafos/%sgrafo.png' alt='Grafo'></center>", name);

        //Grafos Makefile
        fprintf(makefile, "\tdot -Tpng grafos/%sgrafo.dot > grafos/%sgrafo.png\n", name, name);

        //Print grafo hiperlink to index.html
        fprintf(fa, "\t\t<li><a href=\"%sgrafo.html\">%s</a></li></br>\n", name, name);
    }

    //Close index.html
    FILE *fileClose = fopen("out/html/index.html", "a");
    if (fileClose == NULL) {
        printf("Error opening file %s!\n", "out/html/index.html");
        exit(1);
    }
    fprintf(fileClose, "\t</div>\n");
    fprintf(fileClose, "</body>\n");
    fprintf(fileClose, "</html>\n");

    //Close geral grafo file
    FILE *geralGrafoClose = fopen("out/grafos/grafo.dot", "a");
    if(geralGrafoClose == NULL) {
        printf("Error opening file %s!\n", "out/grafos/grafo.dot");
        exit(1);
    }
    fprintf(geralGrafoClose, "}\n");

    g_list_free(concepts);
    fclose(makefile);
}

/*
adciona uma nova entrada ao conjunto de meta relações.
*/
Relation getRelation( Ontology saurus, const char* relationname ){

    Relation result = mkRelation(relationname);

    if( !g_hash_table_contains(saurus->relations, result) &&
            !g_hash_table_contains(saurus->suportedLanguages, relationname) &&
                !g_hash_table_contains(saurus->noteSections, relationname) ){
        g_hash_table_insert(
            saurus->relations,
            mkRelation(relationname),
            newRelationSetfromHashTable()
        );
    }
    return result;
}

Concept getConcept( Ontology saurus, const char* conceptname ){

    if( ! g_hash_table_contains(saurus->concepts, conceptname) ){
        g_hash_table_insert(
            saurus->concepts,
            strdup(conceptname),
            mkConcept(conceptname)
        );
    }

    return (Concept)g_hash_table_lookup(saurus->concepts,conceptname);
}

void relateRaw(Ontology saurus, const char* subclasse, const char* superclasse ){
    Relation a = getRelation( saurus, subclasse);
    Relation b = getRelation( saurus, superclasse);

    relate( saurus, a, b);
}

/*
associa um novo par de relações.
*/

void relate( Ontology saurus, Relation subclasse/*NT - tem de ser avisado */, Relation superclasse/*BT*/){

    if( !g_hash_table_contains(saurus->relations,superclasse) ){
        g_hash_table_insert(saurus->relations, clone(superclasse), g_hash_table_new( hashRelation, equalRelation) );
    }

    if( !g_hash_table_contains(saurus->relations,subclasse) ){
        g_hash_table_insert(saurus->relations, clone(subclasse), g_hash_table_new( hashRelation, equalRelation) );
    }

    GHashTable * rl = g_hash_table_lookup(saurus->relations, superclasse);

    Relation cpy = clone(subclasse);
    g_hash_table_insert(rl, cpy, cpy);
}

void associate(Ontology saurus, Concept source, const char * relation, const char * assoc){

    if( g_hash_table_contains(saurus->suportedLanguages, relation) ){

        translation( source, relation, assoc);

    }else if( g_hash_table_contains(saurus->noteSections, relation) ) {

        note( source, relation, assoc);

    }else{

        Relation r = getRelation(saurus, relation);
        Concept associatee = getConcept(saurus, assoc);

        linkConcepts( source, r, associatee);
        RelationSet rs = g_hash_table_lookup( saurus->relations, r);
        GList * relationlisting = g_hash_table_get_keys(rs);

        for( GList * cur = relationlisting; cur ; cur = cur->next )
            linkConcepts(associatee, (Relation)cur->data, source);

        g_list_free(relationlisting);
    }

}
