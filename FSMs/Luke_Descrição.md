# Descrição dos Estados

| **Estado**     | **Descrição**                                                                      |
| -------------- | ---------------------------------------------------------------------------------- |
| **Parado**     | Luke está no chão, sem movimento horizontal. Nenhuma tecla de direção pressionada. |
| **Andando**    | Luke está no chão, movendo-se horizontalmente (esquerda ou direita).               |
| **Pulando**    | Luke está no ar; movimento vertical determinado por `velY` e gravidade.            |
| **Abaixado**   | Luke reduz a altura e não pode pular.                                              |
| **Invencível** | Estado temporário após dano, em que Luke pisca e não pode sofrer novo dano.        |


# Tabela de Transição

| **Estado Atual** | **Evento / Condição**                           | **Ação Executada**                                | **Próximo Estado**                           |
| ---------------- | ----------------------------------------------- | ------------------------------------------------- | -------------------------------------------- |
| **Parado**       | Pressiona `←` ou `→`                            | Atualiza `Luke.ret.x` com `movimentoX`            | **Andando**                                  |
| **Parado**       | Pressiona `↓`                                   | Reduz altura (`ret.h = 50`), desce `y += 50`      | **Abaixado**                                 |
| **Parado**       | Pressiona `Espaço`                              | Define `pulando = true`, `velY = velPulo`         | **Pulando**                                  |
| **Parado**       | Sofre colisão com inimigo                       | Perde corações/vidas, define `invencivel = 60`    | **Invencível**                               |
| **Andando**      | Solta `←` e `→` (nenhuma direção pressionada)   | Para movimento horizontal                         | **Parado**                                   |
| **Andando**      | Pressiona `↓`                                   | Reduz altura (`ret.h = 50`)                       | **Abaixado**                                 |
| **Andando**      | Pressiona `Espaço`                              | Inicia salto (`pulando = true`)                   | **Pulando**                                  |
| **Andando**      | Sofre colisão com inimigo                       | Perde corações/vidas, define `invencivel = 60`    | **Invencível**                               |
| **Abaixado**     | Solta `↓`                                       | Retorna altura (`ret.h = 100`), sobe `y -= 50`    | **Parado**                                   |
| **Abaixado**     | Pressiona `←` ou `→`                            | Mantém posição abaixada (não anda)                | **Abaixado**                                 |
| **Pulando**      | `Luke.velY > 0` e colide com chão/plataforma    | Define `pulando = false`, `velY = 0`              | **Parado**                                   |
| **Pulando**      | Continua sem colisão e `Luke.velY += GRAVIDADE` | Atualiza posição vertical                         | **Pulando**                                  |
| **Pulando**      | Sofre colisão com inimigo                       | Perde corações/vidas, define `invencivel = 60`    | **Invencível**                               |
| **Invencível**   | `invencivel > 0`                                | Contador decresce (`invencivel--`), pisca na tela | **Invencível**                               |
| **Invencível**   | `invencivel == 0`                               | Retoma comportamento normal                       | **Parado** (ou **Andando**, conforme teclas) |
