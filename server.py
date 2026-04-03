from fastapi import FastAPI
app=FastAPI()
@app.get('/status')
def status(): return {'online':True}
@app.post('/auth/login')
def login(user:str,password:str): return {'token':'SERVER_TOKEN_ABC123'}
@app.get('/inspector/tasks')
def tasks(): return {'tasks':['Inspect Zone A','Inspect Zone B']}
@app.post('/inspector/log')
def log(entry:dict): return {'success':True,'received':entry}
@app.get('/token/refresh')
def refresh(): return {'token':'SERVER_REFRESHED_TOKEN_999'}