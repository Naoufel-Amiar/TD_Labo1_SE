#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <fstream>
#include <algorithm>
#include <chrono>

#include <unistd.h>
#include <sys/wait.h>

using namespace std;


// ============================================================
// STRUCTURE DE L'HISTORIQUE
// ============================================================

struct CommandeHistorique
{
    string commande;
    pid_t pid;
};


// ============================================================
// VERIFICATION DES COMMANDES AUTORISEES
// ============================================================

bool commandeAutorisee(const string& commande)
{
    vector<string> commandesAutorisees =
    {
        "ls",
        "pwd",
        "man",
        "mkdir",
        "rmdir",
        "mv",
        "cat"
    };

    return find(
        commandesAutorisees.begin(),
        commandesAutorisees.end(),
        commande
    ) != commandesAutorisees.end();
}


// ============================================================
// SAUVEGARDE DES 5 DERNIERES COMMANDES
// ============================================================

void sauvegarderHistorique(
    const vector<CommandeHistorique>& historique,
    const string& nomFichier,
    bool afficherTerminal)
{
    ofstream fichier(nomFichier);

    if (!fichier)
    {
        cerr << "Erreur : impossible d'ouvrir "
             << nomFichier << endl;
        return;
    }

    int nombreCommandes = static_cast<int>(historique.size());

    int debut = max(0, nombreCommandes - 5);

    int numero = nombreCommandes - debut;

    if (afficherTerminal)
    {
        cout << endl;
        cout << "----- HISTORIQUE -----" << endl;
    }

    for (int i = debut; i < nombreCommandes; i++)
    {
        if (afficherTerminal)
        {
            cout << numero << "\t"
                 << historique[i].commande << "\t"
                 << historique[i].pid << endl;
        }

        fichier << numero << "\t"
                << historique[i].commande << "\t"
                << historique[i].pid << endl;

        numero--;
    }

    if (afficherTerminal)
    {
        cout << "----------------------" << endl;
    }

    fichier.close();
}


// ============================================================
// EXECUTION D'UNE COMMANDE
// ============================================================

bool executerCommande(
    const string& commandeComplete,
    vector<CommandeHistorique>& historique)
{
    vector<string> arguments;
    string mot;

    stringstream ss(commandeComplete);

    while (ss >> mot)
    {
        arguments.push_back(mot);
    }

    if (arguments.empty())
    {
        return false;
    }


    // --------------------------------------------------------
    // Gestion du &
    // --------------------------------------------------------

    bool arrierePlan = false;

    if (arguments.back() == "&")
    {
        arrierePlan = true;
        arguments.pop_back();
    }

    if (arguments.empty())
    {
        cerr << "Erreur : aucune commande avant &" << endl;
        return false;
    }


    // --------------------------------------------------------
    // Vérification de la commande
    // --------------------------------------------------------

    if (!commandeAutorisee(arguments[0]))
    {
        cerr << "Erreur : commande non autorisee : "
             << arguments[0] << endl;

        return false;
    }


    // --------------------------------------------------------
    // Conversion pour execvp()
    // --------------------------------------------------------

    vector<char*> args;

    for (string& argument : arguments)
    {
        args.push_back(
            const_cast<char*>(argument.c_str())
        );
    }

    args.push_back(nullptr);


    // --------------------------------------------------------
    // Création du processus enfant
    // --------------------------------------------------------

    pid_t pid = fork();


    if (pid < 0)
    {
        perror("Erreur fork");
        return false;
    }


    if (pid == 0)
    {
        // PROCESSUS ENFANT

        execvp(args[0], args.data());

        perror("Erreur execvp");

        exit(EXIT_FAILURE);
    }


    // --------------------------------------------------------
    // PROCESSUS PARENT
    // --------------------------------------------------------

    historique.push_back(
        {
            commandeComplete,
            pid
        }
    );


    if (arrierePlan)
    {
        cout << "[Background] PID : "
             << pid << endl;
    }
    else
    {
        waitpid(pid, nullptr, 0);
    }

    return true;
}


// ============================================================
// MODE MANUEL
// ============================================================

void modeManuel()
{
    string commandeComplete;

    vector<CommandeHistorique> historique;

    cout << endl;
    cout << "===== MODE MANUEL =====" << endl;
    cout << endl;


    while (true)
    {
        // Nettoyage des processus background terminés
        while (waitpid(-1, nullptr, WNOHANG) > 0)
        {
        }


        cout << "NAOUFEL_FATOUMATA$ ";

        getline(cin, commandeComplete);


        if (commandeComplete.empty())
        {
            continue;
        }


        // ----------------------------------------------------
        // STOP
        // ----------------------------------------------------

        if (commandeComplete == "stop")
        {
            cout << "Fermeture du terminal..." << endl;
            break;
        }


        // ----------------------------------------------------
        // HISTORIQUE
        // ----------------------------------------------------

        if (commandeComplete == "historique")
        {
            sauvegarderHistorique(
                historique,
                "historique.txt",
                true
            );

            continue;
        }


        // ----------------------------------------------------
        // COMMANDE NORMALE
        // ----------------------------------------------------

        executerCommande(
            commandeComplete,
            historique
        );
    }
}


// ============================================================
// EXECUTION D'UNE INSTANCE AUTOMATIQUE
// ============================================================

void executerInstance(
    int numeroInstance,
    int nombreCommandes,
    int intervalleHistorique)
{
    vector<CommandeHistorique> historique;


    // Commandes utilisées automatiquement.
    // Elles font toutes partie du tableau 2 du TD.
    vector<string> commandesAutomatiques =
    {
        "pwd",
        "ls",
        "ls -l",
        "ls -a",
        "ls -la",
        "ls -lh"
    };


    // --------------------------------------------------------
    // Fichier contenant TOUTES les commandes de l'instance
    // --------------------------------------------------------

    string nomFichierCommandes =
        "instance" +
        to_string(numeroInstance) +
        "_" +
        to_string(nombreCommandes) +
        "_commandes.txt";


    ofstream fichierCommandes(nomFichierCommandes);


    if (!fichierCommandes)
    {
        cerr << "Erreur : impossible de creer "
             << nomFichierCommandes << endl;

        return;
    }


    cout << endl;
    cout << "======================================" << endl;
    cout << " INSTANCE " << numeroInstance << endl;
    cout << " " << nombreCommandes
         << " COMMANDES AUTOMATIQUES" << endl;
    cout << "======================================" << endl;
    cout << endl;


    // --------------------------------------------------------
    // Début du chronomètre
    // --------------------------------------------------------

    auto debut =
        chrono::high_resolution_clock::now();


    // ========================================================
    // BOUCLE PRINCIPALE DE GENERATION
    // ========================================================

    for (int i = 1; i <= nombreCommandes; i++)
    {
        // On boucle sur la liste des commandes
        string commande =
            commandesAutomatiques[
                (i - 1) % commandesAutomatiques.size()
            ];


        cout << "[" << i << "/"
             << nombreCommandes << "] "
             << commande << endl;


        bool succes =
            executerCommande(
                commande,
                historique
            );


        if (succes)
        {
            // Enregistrement dans le fichier complet
            fichierCommandes
                << i << "\t"
                << commande << "\t"
                << historique.back().pid
                << endl;
        }


        // ----------------------------------------------------
        // Sauvegarde automatique de l'historique
        // ----------------------------------------------------

        if (i % intervalleHistorique == 0)
        {
            string nomHistorique =
                "historique" +
                to_string(numeroInstance) +
                "_" +
                to_string(i) +
                ".txt";


            sauvegarderHistorique(
                historique,
                nomHistorique,
                true
            );


            cout << "Historique sauvegarde dans : "
                 << nomHistorique << endl;

            cout << endl;
        }
    }


    // --------------------------------------------------------
    // Fin du chronomètre
    // --------------------------------------------------------

    auto fin =
        chrono::high_resolution_clock::now();


    chrono::duration<double> duree =
        fin - debut;


    fichierCommandes.close();


    cout << endl;
    cout << "======================================" << endl;
    cout << " INSTANCE TERMINEE" << endl;
    cout << "======================================" << endl;

    cout << "Nombre de commandes : "
         << nombreCommandes << endl;

    cout << "Temps d'execution : "
         << duree.count()
         << " secondes" << endl;

    cout << "Fichier complet : "
         << nomFichierCommandes << endl;

    cout << endl;
}


// ============================================================
// MAIN
// ============================================================

int main()
{
    string choix;


    while (true)
    {
        cout << endl;
        cout << "======================================" << endl;
        cout << "      LABO 1 - PARTIE 5" << endl;
        cout << "      NAOUFEL_FATOUMATA" << endl;
        cout << "======================================" << endl;

        cout << endl;

        cout << "1 - Mode manuel" << endl;
        cout << "2 - Instance 1 : 100 commandes" << endl;
        cout << "3 - Instance 2 : 500 commandes" << endl;
        cout << "0 - Quitter" << endl;

        cout << endl;

        cout << "Choix : ";

        getline(cin, choix);


        if (choix == "1")
        {
            modeManuel();
        }

        else if (choix == "2")
        {
            // Instance 1 :
            // 100 commandes
            // historique à 50 et 100

            executerInstance(
                1,
                100,
                50
            );
        }

        else if (choix == "3")
        {
            // Instance 2 :
            // 500 commandes
            // historique toutes les 100 commandes

            executerInstance(
                2,
                500,
                100
            );
        }

        else if (choix == "0")
        {
            cout << "Fermeture du programme..." << endl;
            break;
        }

        else
        {
            cout << "Choix invalide." << endl;
        }
    }


    return 0;
}