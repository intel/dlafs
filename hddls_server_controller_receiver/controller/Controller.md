# Controller
The controller is a command line interface (CLI) interacting with the remote server.
It supports the following features:
* create/destroy the remote hddlpipe.
* dynamical property setting of the remote hddlpipe
* update inference model of the remote hddlpipe

## Detail Usage
Before execution of controller, please make sure you put the TLS certificates under the `client_cert` folder and obey the following naming rules.

| name | certificate type |
| ---- | ---- |
| ca-cert.pem | the certificate for CA |
| client-cert.pem | the CA signed controller certificate |
| client-key.pem | the controller private key |


After boot up, the CLI will ask the user to input server url. If you want use default url just press enter.
After successful connection, the CLI will prompt `command>` and wait for user's interaction.

Currently, the controller CLI supports the following commands

| Command | Explanation |
| ---- | ---- |
| help | show all commands |
| c <create.json> | create pipelines |
| p <property.json> <pipe_id> | set pipelines property |
| d <destroy.json>  <pipe_id> | destroy pipelines |
| pipe | display pipes belonging to the very client |
| client | display client ID |
| m <model_path> | upload custom file |
| q | exit client |

For example, if you want to create a hddlpipe in the remote side, you can enter

```bash
command> c create.json
```

After hddlpipe creation, the CLI outputs the `pipe_ids` that the controller currently owns.

