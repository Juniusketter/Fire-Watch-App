# Flask server endpoints for edit/delete
from flask import Flask, request, jsonify
app = Flask(__name__)
@app.route('/api/user/<uid>', methods=['PUT'])
def update_user(uid): return jsonify(ok=True)
@app.route('/api/user/<uid>', methods=['DELETE'])
def delete_user(uid): return jsonify(ok=True)
