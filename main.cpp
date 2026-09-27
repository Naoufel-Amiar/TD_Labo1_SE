#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <fstream>
#include <algorithm>

#include <unistd.h>
#include <sys/wait.h>

using namespace std;


// Structure permettant de mémoriser une commande et le PID
// du processus enfant qui l'a exécutée.
struct CommandeHistorique
{
    string commande;
    pid_t pid;
};


// Vérifie si la commande demandée fait partie des commandes
// autorisées dans le laboratoire.
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


// Affiche les 5 dernières commandes exécutées
// et les sauvegarde dans historique.txt.
void afficherHistorique(const vector<CommandeHistorique>& historique)
{
    ofstream fichier("historique.txt");

    if (!fichier)
    {
        cerr << "Erreur : impossible d'ouvrir historique.txt" << endl;
        return;
    }

    int nombreCommandes = historique.size();

    // On affiche au maximum les 5 dernières commandes.
    int debut = max(0, nombreCommandes - 5);

    int numero = nombreCommandes - debut;

    cout << endl;
    cout << "----- HISTORIQUE -----" << endl;

    for (int i = debut; i < nombreCommandes; i++)
    {
        cout << numero << "\t"
             << historique[i].commande << "\t"
             << historique[i].pid << endl;

        fichier << numero << "\t"
                << historique[i].commande << "\t"
                << historique[i].pid << endl;

        numero--;
    }

    cout << "----------------------" << endl;

    fichier.close();
}


int main()
{
    string commandeComplete;

    vector<CommandeHistorique> historique;

    while (true)
    {
        // Permet de nettoyer les processus en arrière-plan terminés
        // sans bloquer le terminal.
        while (waitpid(-1, nullptr, WNOHANG) > 0)
        {
        }

        cout << "NAOUFEL_FATOUMATA$ ";

        getline(cin, commandeComplete);


        // Si l'utilisateur appuie seulement sur Entrée
        if (commandeComplete.empty())
        {
            continue;
        }


        // Commande spéciale permettant de fermer notre terminal
        if (commandeComplete == "stop")
        {
            cout << "Fermeture du terminal..." << endl;
            break;
        }


        // Commande spéciale créée pour le laboratoire
        if (commandeComplete == "historique")
        {
            afficherHistorique(historique);
            continue;
        }


        // ---------------------------------------------------
        // Découpage de la commande en plusieurs arguments
        // ---------------------------------------------------

        vector<string> arguments;
        string mot;

        stringstream ss(commandeComplete);

        while (ss >> mot)
        {
            arguments.push_back(mot);
        }

        if (arguments.empty())
        {
            continue;
        }


        // ---------------------------------------------------
        // Détection du symbole &
        // ---------------------------------------------------

        bool arrierePlan = false;

        if (arguments.back() == "&")
        {
            arrierePlan = true;

            // & ne doit pas être envoyé à execvp()
            arguments.pop_back();
        }


        // Cas d'une entrée contenant seulement &
        if (arguments.empty())
        {
            cerr << "Erreur : aucune commande avant &" << endl;
            continue;
        }


        // ---------------------------------------------------
        // Validation de la commande
        // ---------------------------------------------------

        if (!commandeAutorisee(arguments[0]))
        {
            cerr << "Erreur : commande non autorisee : "
                 << arguments[0] << endl;

            continue;
        }


        // ---------------------------------------------------
        // Conversion pour execvp()
        // ---------------------------------------------------

        vector<char*> args;

        for (string& argument : arguments)
        {
            args.push_back(const_cast<char*>(argument.c_str()));
        }

        // execvp() exige un pointeur NULL en dernier argument
        args.push_back(nullptr);


        // ---------------------------------------------------
        // Création du processus enfant
        // ---------------------------------------------------

        pid_t pid = fork();


        if (pid < 0)
        {
            perror("Erreur fork");
            continue;
        }


        if (pid == 0)
        {
            // =========================
            // PROCESSUS ENFANT
            // =========================

            execvp(args[0], args.data());

            // Si execvp fonctionne, cette ligne
            // n'est normalement jamais atteinte.
            perror("Erreur execvp");

            exit(EXIT_FAILURE);
        }


        // =========================
        // PROCESSUS PARENT
        // =========================

        // Ajout de la commande et du PID dans l'historique
        historique.push_back(
            {
                commandeComplete,
                pid
            }
        );


        if (arrierePlan)
        {
            cout << "[Background] PID : " << pid << endl;
        }
        else
        {
            // En foreground, le parent attend
            // la fin du processus enfant.
            waitpid(pid, nullptr, 0);
        }
    }

    return 0;
}